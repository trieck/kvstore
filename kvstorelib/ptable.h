#pragma once

#include "blockio.h"
#include "fnvhash64.h"
#include "primes.h"
#include "repo.h"

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
#define BUCKET_KEYL(p, b)       ((p)->buckets[b].length)
#define BUCKET_KEY(p, b)        (&(p)->buckets[b].key)
#define BUCKET_DATUM(p, b)      ((p)->buckets[b].datum)

template <typename K, typename V, size_t max_key_length = 128>
class persistent_hash_table
{
public:
    ~persistent_hash_table()
    {
        close();
    }

    bool insert(const K& key, const V& value)
    {
        auto* ppage = page();

        uint64_t pageno, bucket;
        if (!find_slot(ppage, key, pageno, bucket)) {
            return false;
        }

        set_key(ppage, bucket, key);

        uint64_t offset;
        m_repo.insert(value, offset);

        SET_FILLED(ppage, bucket);
        BUCKET_DATUM(ppage, bucket) = offset;

        m_fillcount++;

        m_index.writeblock(pageno, ppage);

        if (isfull()) {
            resize();
        }

        return true;
    }

    bool lookup(const K& key, V& value)
    {
        auto* ppage = page();

        uint64_t pageno, bucket;
        if (!lookup(ppage, key, pageno, bucket)) {
            return false;
        }

        if (IS_DELETED(ppage, bucket)) {
            return false;
        }

        auto offset = BUCKET_DATUM(ppage, bucket);

        m_repo.readVal(offset, value);

        return true;
    }

    void open(const char* idxfile, uint32_t entries = 10000)
    {
        close();

        m_entries = entries;
        m_idxfile = idxfile;

        maketable();

        std::filesystem::path repopath(m_idxfile);
        repopath.replace_extension("dat");

        m_repo.open(repopath.string().c_str());
    }

    void close()
    {
        m_index.close();
        m_repo.close();
    }

    uint64_t indexsize()
    {
        return m_index.fileSize();
    }

    uint64_t tablesize() const
    {
        return m_tablesize;
    }

    uint64_t fillcount() const
    {
        return m_fillcount;
    }

    float loadfactor() const
    {
        if (m_tablesize == 0) {
            return 0;
        }

        return 100 * (static_cast<float>(m_fillcount) / static_cast<float>(m_tablesize));
    }

    std::string indexname() const
    {
        return m_idxfile;
    }

    uint64_t maxrun()
    {
        uint64_t maxrun = 0, bucket = 0, pageno = 0;
        BlockIO::Block block{};

        auto ppage = page();
        auto ppage2 = reinterpret_cast<LPPAGE>(block.data());

        m_index.readblock(pageno, ppage);

        for (; ;) {
            if (IS_FILLED(ppage, bucket)) {
                auto pbucket = &BUCKET(ppage, bucket);
                maxrun = std::max(maxrun, runLength(ppage2, pbucket->key, pbucket->length));
            }

            if ((bucket = ((bucket + 1) % BUCKETS_PER_PAGE)) == 0) {
                // next page
                if ((pageno = ((pageno + 1) % m_nbpages)) == 0) {
                    break; // wrapped
                }

                m_index.readblock(pageno, ppage);
            }
        }

        return maxrun;
    }


private:
#pragma pack (push, 1)
    // hash table bucket
    typedef struct Bucket
    {
        uint32_t flags; // bucket flags
        uint64_t datum; // offset to datum
        uint16_t length; // key length
        uint8_t key[max_key_length]; // key
    }* LPBUCKET;

    static_assert(sizeof(Bucket) < BlockIO::BLOCK_SIZE);

    // hash table page
    typedef struct Page
    {
        Bucket buckets[1]; // hash table
    }* LPPAGE;

#pragma pack (pop)
    static constexpr auto BUCKETS_PER_PAGE = BlockIO::BLOCK_SIZE / sizeof(Bucket);
    static_assert(BUCKETS_PER_PAGE >= 1);

    size_t hash(const void* pkey, size_t keyLength) const
    {
        auto h = fnvhash64(pkey, keyLength) % m_tablesize;
        return h;
    }

    size_t key_size(const K& key) const
    {
        return key.size() * sizeof(typename K::traits_type::char_type);
    }

    size_t hash(const K& key) const
    {
        return hash(key.data(), key_size(key));
    }

    bool isfull() const
    {
        return m_fillcount >= (m_tablesize / 2);
    }

    void resize()
    {
        m_index.flush();

        auto noldpages = m_nbpages;
        m_entries = m_entries * 2;
        maketable(false);

        auto ppage = page();

        BlockIO::Block page2{};
        auto ppage2 = reinterpret_cast<LPPAGE>(page2.data());

        uint64_t bucket = 0, pageno = 0;
        m_index.readblock(pageno, ppage);

        for (;;) {
            if (IS_FILLED(ppage, bucket)) {
                auto pbucket = &BUCKET(ppage, bucket);
                uint64_t newpage = -1, newbucket = -1;

                if (!find_slot(ppage2, pbucket->key, pbucket->length, newpage, newbucket)) {
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

    bool is_bucket_key(LPPAGE ppage, uint64_t bucket, const void* pkey, size_t length) const
    {
        size_t keyLength = BUCKET_KEYL(ppage, bucket);
        auto size = std::min(keyLength, std::min(max_key_length, length));
        auto* pbucket_key = BUCKET_KEY(ppage, bucket);

        return memcmp(pbucket_key, pkey, size) == 0;
    }

    bool is_bucket_key(LPPAGE ppage, uint64_t bucket, const K& key) const
    {
        return is_bucket_key(ppage, bucket, key.data(), key_size(key));
    }

    void set_key(LPPAGE ppage, uint64_t bucket, const K& key) const
    {
        auto size = std::min(max_key_length, key_size(key));
        BUCKET_KEYL(ppage, bucket) = static_cast<uint16_t>(size);

        auto* pbucket_key = BUCKET_KEY(ppage, bucket);
        memcpy(pbucket_key, key.data(), size);
    }

    bool lookup(LPPAGE ppage, const K& key, uint64_t& pageno, uint64_t& bucket)
    {
        auto h = hash(key);
        pageno = h / BUCKETS_PER_PAGE;
        bucket = h % BUCKETS_PER_PAGE;

        m_index.readblock(pageno, ppage);

        for (auto i = 0ULL; i < m_tablesize; ++i) {
            if (IS_EMPTY(ppage, bucket)) {
                return false; // no hit
            }

            if (is_bucket_key(ppage, bucket, key)) {
                return true; // hit!
            }

            nextbucket(ppage, bucket, pageno);
        }

        return false;
    }

    bool find_slot(LPPAGE ppage, const void* pkey, size_t keyLength, uint64_t& pageno, uint64_t& bucket)
    {
        auto h = hash(pkey, keyLength);
        pageno = h / BUCKETS_PER_PAGE;
        bucket = h % BUCKETS_PER_PAGE;

        m_index.readblock(pageno, ppage);

        if (IS_EMPTY(ppage, bucket)) {
            return true;
        }

        for (auto i = 0ULL; i < m_tablesize; ++i) {
            if (IS_EMPTY(ppage, bucket)) {
                return true; // empty slot
            }

            if (is_bucket_key(ppage, bucket, pkey, keyLength)) {
                return false; // already exists
            }

            nextbucket(ppage, bucket, pageno);
        }

        return false;
    }

    bool find_slot(LPPAGE ppage, const K& key, uint64_t& pageno, uint64_t& bucket)
    {
        return find_slot(ppage, key.data(), key_size(key), pageno, bucket);
    }

    void nextbucket(LPPAGE ppage, uint64_t& bucket, uint64_t& pageno)
    {
        auto realbucket = BUCKETS_PER_PAGE * pageno + bucket;
        auto nextbucket = (realbucket + 1) % m_tablesize;
        auto nextpage = nextbucket / BUCKETS_PER_PAGE;
        bucket = nextbucket % BUCKETS_PER_PAGE;
        if (pageno != nextpage) {
            m_index.readblock(nextpage, ppage);
            pageno = nextpage;
        }
    }

    void maketable(bool create = true)
    {
        m_tablesize = Primes::prime(m_entries);
        m_nbpages = (m_tablesize / BUCKETS_PER_PAGE) + 1;

        if (create) {
            m_index.open(m_idxfile.c_str(), std::ios::in | std::ios::out | std::ios::trunc);
        }

        m_index.resize(m_nbpages);
    }

    LPPAGE page()
    {
        return reinterpret_cast<LPPAGE>(m_block.data());
    }

    uint64_t runLength(LPPAGE ppage, const void* pkey, size_t length)
    {
        auto h = hash(pkey, length);
        auto pageno = h / BUCKETS_PER_PAGE;
        auto bucket = h % BUCKETS_PER_PAGE;

        m_index.readblock(pageno, ppage);
        if (IS_EMPTY(ppage, bucket)) {
            return 0;
        }

        auto run = 1ULL;
        for (auto i = 0ULL; i < m_tablesize; ++i, ++run) {
            if (IS_EMPTY(ppage, bucket)) {
                break;
            }

            nextbucket(ppage, bucket, pageno);
        }

        return run;
    }

    uint64_t m_tablesize = 0; // table size (prime)
    uint64_t m_fillcount = 0; // fill count
    uint64_t m_nbpages = 0; // number of bucket pages
    uint32_t m_entries = 0; // number of entries requested
    BlockIO m_index; // index block i/o
    BlockIO::Block m_block{}; // disk page
    std::string m_idxfile; // indexfile
    repository<V> m_repo; // value repository
};