#pragma once
#include <concepts>

using offset_t = size_t;
using name_t = char const*;
using description_t = char const*;

template<class T> concept DataStreamRecord = requires(T r, name_t name, offset_t offset) {
  { r.get_bool(name) } -> std::same_as<bool>;
  { r.get_bool(offset) } -> std::same_as<bool>;
};


class record {

public:
  bool get_bool(name_t field_name);
  bool get_bool(offset_t field_offset);

};

template <DataStreamRecord RecordType> class window {

public:
    enum temporal_status {
        active,
        resolved
    };

    enum access_status {
        consumed,
        unconsumed
    };

    auto begin() const { return records_.cbegin(); }
    auto end() const { return records_.cend(); }

    void subscribe();
    temporal_status temporal_status();

    // class std::vector<record const*>::const_iterator begin_{}, end_{};
    std::vector<RecordType const*> records_;

};

static_assert(DataStreamRecord<record>);
window<record> w;