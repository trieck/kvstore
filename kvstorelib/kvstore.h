#pragma once
#include <flatbuffers/flatbuffers.h>

#include "blockio.h"
#include "randperm.h"
#include "repo.h"
#include "sha1.h"

class kvstore
{
public:
    kvstore() = default;
    ~kvstore();

    using FBBuilder = flatbuffers::FlatBufferBuilder;

    bool insert(LPCWSTR key, const FBBuilder& value);
    bool insert(LPCSTR key, const FBBuilder& value);
    bool lookup(LPCWSTR key, FBBuilder& value);
    bool lookup(LPCSTR key, FBBuilder& value);

    void close();
    void open(LPCWSTR idxfile, uint32_t entries = DEFAULT_ENTRIES);
    void unlink();

    static constexpr auto DEFAULT_ENTRIES = 10000UL;

private:
    using digest_type = boost::uuids::detail::sha1::digest_type;

    static bool isEqualDigest(const digest_type& d1, const digest_type& d2);

    bool findSlot(const digest_type& digest, uint64_t& pageno, uint64_t& bucket);
    bool findSlot(LPCSTR key, uint64_t& pageno, uint64_t& bucket);
    bool getBucket(LPCSTR key, uint64_t& pageno, uint64_t& bucket);
    bool isfull() const;
    uint64_t hash(const digest_type& digest) const;
    uint64_t hash(const digest_type& digest, uint64_t size) const;
    uint64_t hash(LPCSTR s) const;
    uint64_t perm(uint64_t i) const;
    void getDigest(uint64_t bucket, digest_type& digest) const;
    void mktable(LPCTSTR idxfile, uint32_t entries);
    void nextbucket(uint64_t i, uint64_t& bucket, uint64_t& pageno);
    void resize();
    void setKey(uint64_t bucket, const digest_type& digest);
    void setKey(uint64_t bucket, LPCSTR key);

    BlockIO m_index; // index block i/o
    Repository m_repo; // data repository
    RandomPerm m_perm; // random permutation for pseudo-random probing
    uint64_t m_tablesize = 0; // size of hash table
    uint64_t m_fillcount = 0; // fill count
    uint64_t m_nbpages = 0; // number of bucket pages
    BlockIO::Block m_page{}; // disk page
};
