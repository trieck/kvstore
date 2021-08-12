#include "kvstorelib.h"
#include "kvstore.h"

#include "fnvhash64.h"
#include "primes.h"
#include "sha1.h"

// ensure one byte alignment for structures below
#pragma pack (push, 1)

using digest_type = boost::uuids::detail::sha1::digest_type;
constexpr auto SHA1_DIGEST_INTS = sizeof(digest_type) / sizeof(uint32_t);

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

// restore default structure alignment
#pragma pack (pop)

// bucket flags                 
#define BF_FILLED               (1 << 0)
#define BF_DELETED              (1 << 1)

// bucket macros
#define IS_EMPTY(p, b)          (!((p)->buckets[b].flags & BF_FILLED))
#define IS_FILLED(p, b)         ((p)->buckets[b].flags & BF_FILLED)
#define SET_FILLED(p, b)        ((p)->buckets[b].flags |= BF_FILLED)
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

void kvstore::open(LPCTSTR idxfile, uint32_t entries)
{
    close();
    mktable(idxfile, entries);

    std::filesystem::path repopath(idxfile);
    repopath.replace_extension("dat");

    m_repo.open(repopath.c_str());
}

bool kvstore::insert(LPCSTR key)
{
    uint64_t pageno, bucket;
    if (!findSlot(key, pageno, bucket)) {
        return false;
    }

    return true;
}

bool kvstore::findSlot(LPCSTR key, uint64_t& pageno, uint64_t& bucket)
{
    uint32_t digest[SHA1_DIGEST_INTS];
    sha1(key, digest);

    return findSlot(digest, pageno, bucket);
}

bool kvstore::findSlot(const digest_type& digest, uint64_t& pageno, uint64_t& bucket)
{
    auto h = hash(digest);
    pageno = h / BUCKETS_PER_PAGE;
    bucket = h % BUCKETS_PER_PAGE;

    auto ppage = reinterpret_cast<LPPAGE>(m_page.data());

    m_index.readblock(pageno, ppage);

    if (IS_EMPTY(ppage, bucket)) {
        return true;
    }

    return false;
}

uint64_t kvstore::hash(const digest_type& digest, uint64_t size) const
{
    return fnvhash64<digest_type>(digest) % size;
}

uint64_t kvstore::hash(const digest_type& digest) const
{
    return hash(digest, m_tablesize);
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

void kvstore::mktable(LPCTSTR idxfile, uint32_t entries)
{
    m_tablesize = Primes::prime(entries);
    m_nbpages = (m_tablesize / BUCKETS_PER_PAGE) + 1;

    m_perm.generate(m_tablesize);

    m_index.open(idxfile, std::ios::in | std::ios::out | std::ios::trunc);
    m_index.writeblock(m_nbpages - 1, m_page.data());
    m_index.flush();
}
