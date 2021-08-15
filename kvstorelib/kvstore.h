#pragma once
#include "blockio.h"
#include "repo.h"
#include "sha1.h"
#include "value.h"

class kvstore
{
public:
    kvstore() = default;
    ~kvstore();

    bool insert(LPCWSTR key, const IValue& value);
    bool insert(LPCSTR key, const IValue& value);
    bool lookup(LPCWSTR key, IValue& value);
    bool lookup(LPCSTR key, IValue& value);
    bool destroy(LPCWSTR key);
    bool destroy(LPCSTR key);

    void close();
    void open(LPCWSTR idxfile, uint32_t entries = DEFAULT_ENTRIES);
    void unlink();

    uint64_t tablesize() const;
    uint64_t maxrun();
    uint64_t indexsize();
    uint64_t fillcount() const;
    std::wstring indexname() const;
    float loadfactor() const;

    static constexpr auto DEFAULT_ENTRIES = 10000UL;

private:
    using digest_type = boost::uuids::detail::sha1::digest_type;

    static bool isEqualDigest(const digest_type& d1, const digest_type& d2);

    bool findSlot(void* pvpage, const digest_type& digest, uint64_t& pageno, uint64_t& bucket);
    bool findSlot(const digest_type& digest, uint64_t& pageno, uint64_t& bucket);
    bool findSlot(LPCSTR key, uint64_t& pageno, uint64_t& bucket);
    bool getBucket(const digest_type& digest, uint64_t& pageno, uint64_t& bucket);
    bool getBucket(LPCSTR key, uint64_t& pageno, uint64_t& bucket);
    bool isfull() const;
    uint64_t hash(const digest_type& digest) const;
    uint64_t hash(const digest_type& digest, uint64_t size) const;
    uint64_t hash(LPCSTR s) const;
    void getDigest(const void* pvpage, uint64_t bucket, digest_type& digest) const;
    void getDigest(uint64_t bucket, digest_type& digest) const;
    void mktable(bool create = true);
    void nextbucket(uint64_t i, uint64_t& bucket, uint64_t& pageno);
    void nextbucket(void* pvpage, uint64_t i, uint64_t& bucket, uint64_t& pageno);
    void resize();
    uint64_t runLength(void* pvpage, const digest_type& digest);
    void setKey(uint64_t bucket, const digest_type& digest);
    void setKey(uint64_t bucket, LPCSTR key);

    BlockIO m_index; // index block i/o
    Repository m_repo; // data repository
    uint64_t m_tablesize = 0; // size of hash table
    uint64_t m_entrysize = 0; // requested entry size
    uint64_t m_fillcount = 0; // fill count
    uint64_t m_nbpages = 0; // number of bucket pages
    BlockIO::Block m_page{}; // disk page
    std::wstring m_idxfile; // index filename
};
