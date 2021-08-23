#pragma once

#include "blockstrm.h"
#include "fnvhash64.h"
#include "primes.h"
#include "repo.h"
#include "storage.h"

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
#define BUCKET_KEYSIZE(p, b)    ((p)->buckets[b].size)
#define BUCKET_KEY(p, b)        (&(p)->buckets[b].key)
#define BUCKET_DATUM(p, b)      ((p)->buckets[b].datum)

template <typename K, typename V, size_t max_key_size = 128>
class persistent_hash_table
{
public:
    ~persistent_hash_table()
    {
        close();
    }

    std::wstring filename() const
    {
        return m_storage.filename();
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

    void create(LPCWSTR filename, uint32_t entries = 10000)
    {
        m_entries = entries;

        close();
        m_storage.create(filename);

        makeindex();

        m_repo.create(m_storage, L"repository");
    }

    void close()
    {
        m_index.close();
        m_repo.close();
        m_storage.close();
    }

    uint64_t storagesize()
    {
        return m_storage.size();
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

    uint64_t maxrun()
    {
        uint64_t maxrun = 0, bucket = 0, pageno = 0;
        BlockStream::Block block{};

        auto ppage = page();
        auto ppage2 = reinterpret_cast<LPPAGE>(block.data());

        m_index.readblock(pageno, ppage);

        for (; ;) {
            if (IS_FILLED(ppage, bucket)) {
                auto pbucket = &BUCKET(ppage, bucket);
                maxrun = std::max(maxrun, run_length(ppage2, pbucket->key, pbucket->size));
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

    void iterate(const std::function<void(const K& key, const V& value)>& fn)
    {
        uint64_t pageno = 0, bucket = 0;

        auto ppage = page();

        m_index.readblock(pageno, ppage);

        for (;;) {
            if (IS_FILLED(ppage, bucket)) {
                auto pbucket = &BUCKET(ppage, bucket);
                K key(key_ptr(pbucket->key), key_length(pbucket->size));
                V value;
                m_repo.readVal(pbucket->datum, value);

                fn(key, value);
            }

            if ((bucket = ((bucket + 1) % BUCKETS_PER_PAGE)) == 0) {
                // next page
                if ((pageno = ((pageno + 1) % m_nbpages)) == 0) {
                    break; // wrapped
                }

                m_index.readblock(pageno, ppage);
            }
        }
    }

private:
#pragma pack (push, 1)
    // hash table bucket
    typedef struct Bucket
    {
        uint32_t flags;            // bucket flags
        uint64_t datum;            // offset to datum
        uint16_t size;             // key size
        uint8_t key[max_key_size]; // key
    }* LPBUCKET;

    static_assert(sizeof(Bucket) < BlockStream::BLOCK_SIZE);

    // hash table page
    typedef struct Page
    {
        Bucket buckets[1]; // hash table
    }* LPPAGE;

#pragma pack (pop)
    static constexpr auto BUCKETS_PER_PAGE = BlockStream::BLOCK_SIZE / sizeof(Bucket);
    static_assert(BUCKETS_PER_PAGE >= 1);

    using key_char_type = typename K::traits_type::char_type;

    size_t hash(const void* pkey, size_t keyLength) const
    {
        auto h = fnvhash64(pkey, keyLength) % m_tablesize;
        return h;
    }

    const key_char_type* key_ptr(void* pkey) const
    {
        return static_cast<const key_char_type*>(pkey);
    }

    size_t key_size(const K& key) const
    {
        return key.size() * sizeof(key_char_type);
    }

    size_t key_length(uint16_t size) const
    {
        return size / sizeof(key_char_type);
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
        makeindex(false);

        auto ppage = page();

        BlockStream::Block page2{};
        auto ppage2 = reinterpret_cast<LPPAGE>(page2.data());

        uint64_t bucket = 0, pageno = 0;
        m_index.readblock(pageno, ppage);

        for (;;) {
            if (IS_FILLED(ppage, bucket)) {
                auto pbucket = &BUCKET(ppage, bucket);
                uint64_t newpage = -1, newbucket = -1;

                if (!find_slot(ppage2, pbucket->key, pbucket->size, newpage, newbucket)) {
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

    bool is_bucket_key(LPPAGE ppage, uint64_t bucket, const void* pkey, size_t sz) const
    {
        size_t keySize = BUCKET_KEYSIZE(ppage, bucket);
        auto size = std::min(keySize, std::min(max_key_size, sz));
        auto* pbucket_key = BUCKET_KEY(ppage, bucket);

        return memcmp(pbucket_key, pkey, size) == 0;
    }

    bool is_bucket_key(LPPAGE ppage, uint64_t bucket, const K& key) const
    {
        return is_bucket_key(ppage, bucket, key.data(), key_size(key));
    }

    void set_key(LPPAGE ppage, uint64_t bucket, const K& key) const
    {
        auto size = std::min(max_key_size, key_size(key));
        BUCKET_KEYSIZE(ppage, bucket) = static_cast<uint16_t>(size);

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

    void makeindex(bool create = true)
    {
        ATLASSERT(m_storage.isOpen());

        m_tablesize = Primes::prime(m_entries);
        m_nbpages = (m_tablesize / BUCKETS_PER_PAGE) + 1;

        if (create) {
            m_index.create(m_storage, L"index");
        }

        m_index.resize(m_nbpages);
    }

    LPPAGE page()
    {
        return reinterpret_cast<LPPAGE>(m_block.data());
    }

    uint64_t run_length(LPPAGE ppage, const void* pkey, size_t length)
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
    uint64_t m_nbpages = 0;   // number of bucket pages
    uint32_t m_entries = 0;   // number of entries requested

    Storage m_storage;            // structured storage
    BlockStream m_index;          // index block stream
    BlockStream::Block m_block{}; // disk page
    repository<V> m_repo;         // value repository
};
