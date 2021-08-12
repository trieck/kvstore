#pragma once
#include "blockio.h"
#include "randperm.h"
#include "repo.h"
#include "sha1.h"

class kvstore
{
public:
    kvstore() = default;
    ~kvstore();

    void open(LPCWSTR idxfile, uint32_t entries = DEFAULT_ENTRIES);
    bool insert(LPCSTR key);

    void close();
    void unlink();

    static constexpr auto DEFAULT_ENTRIES = 10000UL;

private:
    using digest_type = boost::uuids::detail::sha1::digest_type;

    void mktable(LPCTSTR idxfile, uint32_t entries);
    bool findSlot(LPCSTR key, uint64_t& pageno, uint64_t& bucket);
    bool findSlot(const digest_type& digest, uint64_t& pageno, uint64_t& bucket);
    uint64_t hash(const digest_type& digest) const;
    uint64_t hash(const digest_type& digest, uint64_t size) const;

    BlockIO m_index; // index block i/o
    Repository m_repo; // data repository
    RandomPerm m_perm; // random permutation for pseudo-random probing
    uint64_t m_tablesize = 0; // size of hash table
    uint64_t m_nbpages = 0; // number of bucket pages
    BlockIO::Block m_page{}; // disk page
};
