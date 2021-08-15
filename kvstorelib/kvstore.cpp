#include "kvstorelib.h"
#include "kvstore.h"
#include "fnvhash64.h"
#include "primes.h"
#include "sha1.h"
#include "utf8str.h"

using digest_type = boost::uuids::detail::sha1::digest_type;
constexpr auto SHA1_DIGEST_INTS = sizeof(digest_type) / sizeof(uint32_t);

// ensure one byte alignment for structures below
#pragma pack (push, 1)

// hash table bucket
typedef struct Bucket
{
    uint32_t flags; // bucket flags
    uint64_t datum; // offset to datum
    uint32_t digest[SHA1_DIGEST_INTS]; // sha-1 digest
}* LPBUCKET;

// hash table page
typedef struct Page
{
    Bucket buckets[1]; // hash table
}* LPPAGE;

using LPCPAGE = const Page*;

// restore default structure alignment
#pragma pack (pop)

// bucket flags                 
#define BF_FILLED               (1 << 0)
#define BF_DELETED              (1 << 1)

// bucket macros
#define IS_EMPTY(p, b)          (!((p)->buckets[b].flags & BF_FILLED))
#define IS_FILLED(p, b)         ((p)->buckets[b].flags & BF_FILLED)
#define SET_FILLED(p, b)        ((p)->buckets[b].flags |= BF_FILLED)
#define SET_EMPTY(p, b)         ((p)->buckets[b].flags &= ~BF_FILLED)
#define IS_DELETED(p, b)        ((p)->buckets[b].flags & BF_DELETED)
#define SET_DELETED(p, b)       ((p)->buckets[b].flags |= BF_DELETED)
#define BUCKET(p, b)            ((p)->buckets[b])
#define BUCKET_DIGEST(p, b)     ((p)->buckets[b].digest)
#define BUCKET_DATUM(p, b)      ((p)->buckets[b].datum)

// number of buckets on a page
constexpr auto BUCKETS_PER_PAGE = BlockIO::BLOCK_SIZE / sizeof(Bucket);

kvstore::~kvstore()
{
    close();
}

bool kvstore::insert(LPCWSTR key, const IValue& value)
{
    auto keyA = utf8str(key);

    return insert(static_cast<LPCSTR>(keyA), value);
}

void kvstore::open(LPCWSTR idxfile, uint32_t entries)
{
    m_entrysize = entries;
    m_idxfile = idxfile;

    close();
    mktable(entries);

    std::filesystem::path repopath(m_idxfile);
    repopath.replace_extension("dat");

    m_repo.open(repopath.c_str());
}

void kvstore::setKey(uint64_t bucket, const digest_type& digest)
{
    auto ppage = reinterpret_cast<LPPAGE>(m_page.data());

    auto* bdigest = BUCKET_DIGEST(ppage, bucket);
    memcpy(bdigest, digest, sizeof(digest_type));
}

void kvstore::setKey(uint64_t bucket, LPCSTR key)
{
    uint32_t digest[SHA1_DIGEST_INTS];
    sha1(key, digest);

    setKey(bucket, digest);
}

bool kvstore::isfull() const
{
    return m_fillcount >= (m_tablesize / 2);
}

void kvstore::resize()
{
    m_index.flush();

    auto noldpages = m_nbpages;
    m_entrysize = static_cast<uint32_t>(m_entrysize * 2);
    mktable(false);

    auto ppage = reinterpret_cast<LPPAGE>(m_page.data());

    BlockIO::Block page2{};
    auto ppage2 = reinterpret_cast<LPPAGE>(page2.data());

    uint64_t bucket = 0, pageno = 0;
    m_index.readblock(pageno, ppage);

    for (;;) {
        if (IS_FILLED(ppage, bucket)) {
            auto pbucket = &BUCKET(ppage, bucket);
            uint64_t newpage = -1, newbucket = -1;

            if (!findSlot(ppage2, pbucket->digest, newpage, newbucket)) {
                if (!(newpage == pageno && newbucket == bucket)) {
                    throw std::runtime_error("table full.");
                }
            }

            // rehashed to new location
            if (newpage != pageno && newbucket != bucket) {
                BUCKET(ppage2, newbucket) = *pbucket;
                *pbucket = {}; // clear existing bucket

                m_index.writeblock(pageno, ppage);
                m_index.writeblock(newpage, ppage2);
            } else if (newpage == pageno && newbucket != bucket) {
                BUCKET(ppage, newbucket) = *pbucket;
                BUCKET(ppage, bucket) = {};
                m_index.writeblock(pageno, ppage);
            }
        }

        if ((bucket = (bucket + 1) % BUCKETS_PER_PAGE) == 0) {
            // next page
            if (++pageno >= noldpages) {
                break;
            }

            m_index.readblock(pageno, ppage);
        }
    }
}

bool kvstore::insert(LPCSTR key, const IValue& value)
{
    uint32_t digest[SHA1_DIGEST_INTS];
    sha1(key, digest);

    uint64_t pageno, bucket;
    if (!findSlot(digest, pageno, bucket)) {
        return false;
    }

    setKey(bucket, digest);

    uint64_t offset;
    m_repo.insert(value, offset);

    auto ppage = reinterpret_cast<LPPAGE>(m_page.data());
    SET_FILLED(ppage, bucket);
    BUCKET_DATUM(ppage, bucket) = offset;

    m_fillcount++;

    m_index.writeblock(pageno, ppage);

    if (isfull()) {
        resize();
    }

    return true;
}

bool kvstore::lookup(LPCWSTR key, IValue& value)
{
    auto keyA = utf8str(key);

    return lookup(static_cast<LPCSTR>(keyA), value);
}

bool kvstore::getBucket(const digest_type& digest, uint64_t& pageno, uint64_t& bucket)
{
    auto h = hash(digest);
    pageno = h / BUCKETS_PER_PAGE;
    bucket = h % BUCKETS_PER_PAGE;

    auto ppage = reinterpret_cast<LPPAGE>(m_page.data());

    m_index.readblock(pageno, ppage);

    uint32_t bdigest[SHA1_DIGEST_INTS];
    for (auto i = 0ULL; i < m_tablesize; ++i) {
        if (IS_EMPTY(ppage, bucket)) {
            return false; // no hit
        }

        getDigest(bucket, bdigest);
        if (isEqualDigest(bdigest, digest)) {
            return true; // hit
        }

        nextbucket(ppage, i, bucket, pageno);
    }

    return false;
}

bool kvstore::getBucket(LPCSTR key, uint64_t& pageno, uint64_t& bucket)
{
    uint32_t digest[SHA1_DIGEST_INTS];
    sha1(key, digest);

    return getBucket(digest, pageno, bucket);
}

bool kvstore::lookup(LPCSTR key, IValue& value)
{
    uint64_t pageno, bucket;
    if (!getBucket(key, pageno, bucket)) {
        return false;
    }

    auto ppage = reinterpret_cast<LPCPAGE>(m_page.data());

    if (IS_DELETED(ppage, bucket)) {
        return false;
    }

    auto offset = BUCKET_DATUM(ppage, bucket);

    m_repo.readVal(offset, value);

    return true;
}

bool kvstore::destroy(LPCWSTR key)
{
    auto keyA = utf8str(key);
    return destroy(static_cast<LPCSTR>(keyA));
}

bool kvstore::destroy(LPCSTR key)
{
    uint64_t pageno, bucket;
    if (!getBucket(key, pageno, bucket)) {
        return false;
    }

    auto ppage = reinterpret_cast<LPPAGE>(m_page.data());

    SET_DELETED(ppage, bucket);

    m_index.writeblock(pageno, ppage);

    return true;
}

void kvstore::getDigest(const void* pvpage, uint64_t bucket, digest_type& digest) const
{
    auto ppage = static_cast<LPCPAGE>(pvpage);

    auto* bdigest = BUCKET_DIGEST(ppage, bucket);
    memcpy(digest, bdigest, sizeof(digest_type));
}

void kvstore::getDigest(uint64_t bucket, digest_type& digest) const
{
    getDigest(m_page.data(), bucket, digest);
}

bool kvstore::isEqualDigest(const digest_type& d1, const digest_type& d2)
{
    return memcmp(d1, d2, sizeof(digest_type)) == 0;
}

uint64_t kvstore::perm(uint64_t i) const
{
    return 1 + m_perm[i]; // pseudo-random probing
}

void kvstore::nextbucket(void* pvpage, uint64_t i, uint64_t& bucket, uint64_t& pageno)
{
    auto ppage = static_cast<LPPAGE>(pvpage);

    auto realbucket = BUCKETS_PER_PAGE * pageno + bucket;
    auto nextbucket = (realbucket + perm(i)) % m_tablesize;
    auto nextpage = nextbucket / BUCKETS_PER_PAGE;
    bucket = nextbucket % BUCKETS_PER_PAGE;
    if (pageno != nextpage) {
        m_index.readblock(nextpage, ppage);
        pageno = nextpage;
    }
}

void kvstore::nextbucket(uint64_t i, uint64_t& bucket, uint64_t& pageno)
{
    nextbucket(m_page.data(), i, bucket, pageno);
}

bool kvstore::findSlot(void* pvpage, const digest_type& digest, uint64_t& pageno, uint64_t& bucket)
{
    auto h = hash(digest);
    pageno = h / BUCKETS_PER_PAGE;
    bucket = h % BUCKETS_PER_PAGE;

    auto ppage = static_cast<LPPAGE>(pvpage);

    m_index.readblock(pageno, ppage);

    if (IS_EMPTY(ppage, bucket)) {
        return true;
    }

    uint32_t bdigest[SHA1_DIGEST_INTS];
    for (auto i = 0ULL; i < m_tablesize; ++i) {
        if (IS_EMPTY(ppage, bucket)) {
            return true; // empty slot
        }

        getDigest(ppage, bucket, bdigest);
        if (isEqualDigest(bdigest, digest)) {
            return false; // already exists
        }

        nextbucket(ppage, i, bucket, pageno);
    }

    return false;
}

bool kvstore::findSlot(const digest_type& digest, uint64_t& pageno, uint64_t& bucket)
{
    return findSlot(m_page.data(), digest, pageno, bucket);
}

bool kvstore::findSlot(LPCSTR key, uint64_t& pageno, uint64_t& bucket)
{
    uint32_t digest[SHA1_DIGEST_INTS];
    sha1(key, digest);

    return findSlot(digest, pageno, bucket);
}

uint64_t kvstore::hash(const digest_type& digest, uint64_t size) const
{
    return fnvhash64<digest_type>(digest) % size;
}

uint64_t kvstore::hash(const digest_type& digest) const
{
    return hash(digest, m_tablesize);
}

uint64_t kvstore::hash(LPCSTR s) const
{
    uint32_t digest[SHA1_DIGEST_INTS];
    sha1(s, digest);
    return hash(digest);
}

void kvstore::close()
{
    m_repo.close();
    m_index.close();
}

void kvstore::unlink()
{
    close();
    m_repo.unlink();
    m_index.unlink();
}

uint64_t kvstore::indexsize()
{
    // TODO:
    return 0;
}

uint64_t kvstore::tablesize() const
{
    // TODO:
    return 0;
}

uint64_t kvstore::fillcount() const
{
    // TODO:
    return 0;
}

float kvstore::loadfactor() const
{
    // TODO:
    return 0;
}

uint64_t kvstore::maxrun()
{
    // TODO:
    return 0;
}

void kvstore::mktable(bool create)
{
    m_tablesize = Primes::prime(m_entrysize);
    m_nbpages = (m_tablesize / BUCKETS_PER_PAGE) + 1;
    m_perm.generate(m_tablesize);

    if (create) {
        m_index.open(m_idxfile.c_str(), std::ios::in | std::ios::out | std::ios::trunc);
    }

    m_index.resize(m_nbpages);
}
