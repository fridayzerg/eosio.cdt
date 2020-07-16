#pragma once
#include "../../core/eosio/datastream.hpp"
#include "../../core/eosio/name.hpp"
#include "../../core/eosio/utility.hpp"
#include "../../core/eosio/varint.hpp"

#include <algorithm>
#include <cctype>
#include <functional>

#define EOSIO_CDT_GET_RETURN_T(value_class, index_name) std::decay_t<decltype(std::invoke(&value_class::index_name, std::declval<const value_class*>()))>

/**
 * @brief Macro to define an index.
 * @details This macro allows users to conveniently define an index without having to specify
 * the index template type, as those can be large/unwieldy to type out. It can be used for both primary and secondary indexes.
 *
 * @param index_name    - The index name.
 * @param member_name   - The name of the member pointer used for the index. This also defines the index's C++ variable name.
 */
#define KV_NAMED_INDEX(index_name, member_name)                                                                        \
   index<EOSIO_CDT_GET_RETURN_T(value_type, member_name)> member_name{eosio::name{index_name}, &value_type::member_name};

namespace eosio {
   namespace internal_use_do_not_use {
      extern "C" {
         __attribute__((eosio_wasm_import))
         int64_t kv_erase(uint64_t db, uint64_t contract, const char* key, uint32_t key_size);

         __attribute__((eosio_wasm_import))
         int64_t kv_set(uint64_t db, uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size);

         __attribute__((eosio_wasm_import))
         bool kv_get(uint64_t db, uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size);

         __attribute__((eosio_wasm_import))
         uint32_t kv_get_data(uint64_t db, uint32_t offset, char* data, uint32_t data_size);

         __attribute__((eosio_wasm_import))
         uint32_t kv_it_create(uint64_t db, uint64_t contract, const char* prefix, uint32_t size);

         __attribute__((eosio_wasm_import))
         void kv_it_destroy(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_status(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_compare(uint32_t itr_a, uint32_t itr_b);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_key_compare(uint32_t itr, const char* key, uint32_t size);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_move_to_end(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_next(uint32_t itr, uint32_t& found_key_size = (uint32_t&)std::move(uint32_t(0)), uint32_t& found_value_size = (uint32_t&)std::move(uint32_t(0)));

         __attribute__((eosio_wasm_import))
         int32_t kv_it_prev(uint32_t itr, uint32_t& found_key_size = (uint32_t&)std::move(uint32_t(0)), uint32_t& found_value_size = (uint32_t&)std::move(uint32_t(0)));

         __attribute__((eosio_wasm_import))
         int32_t kv_it_lower_bound(uint32_t itr, const char* key, uint32_t size, uint32_t& found_key_size = (uint32_t&)std::move(uint32_t(0)), uint32_t& found_value_size = (uint32_t&)std::move(uint32_t(0)));

         __attribute__((eosio_wasm_import))
         int32_t kv_it_key(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_value(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
      }
   }

namespace detail {
   constexpr inline size_t max_stack_buffer_size = 512;

   template <typename V>
   static void serialize(const V& value, void* buffer, size_t size) {
      datastream<char*> ds((char*)buffer, size);
      unsigned_int i{0};
      ds << i;
      ds << value;
   }

   template <typename... Vs>
   static void serialize(const std::variant<Vs...>& value, void* buffer, size_t size) {
      datastream<char*> ds((char*)buffer, size);
      ds << value;
   }

   template <typename V>
   static void deserialize(V& value, const void* buffer, size_t size) {
      unsigned_int idx;
      datastream<const char*> ds((char*)buffer, size);

      ds >> idx;
      eosio::check(idx==unsigned_int(0), "there was an error deserializing this value.");
      ds >> value;
   }

   template <typename... Vs>
   static void deserialize(std::variant<Vs...>& value, const void* buffer, size_t size) {
      datastream<const char*> ds((char*)buffer, size);
      ds >> value;
   }

   template <typename V>
   static size_t get_size(const V& value) {
      auto size = pack_size(value);
      return size + 1;
   }

   template <typename... Vs>
   static size_t get_size(const std::variant<Vs...>& value) {
      auto size = pack_size(value);
      return size;
   }
}

/**
 * The key_type struct is used to store the binary representation of a key.
 */
struct key_type : private std::string {
   key_type() = default;
   key_type(const char* c, size_t s) : std::string(c, s) {}

   key_type operator+(const key_type& b) const {
      key_type ret = *this;
      ret += b;
      return ret;
   }

   bool operator==(const key_type& b) const {
      return size() != b.size() && memcmp(data(), b.data(), b.size()) == 0;
   }

   using std::string::data;
   using std::string::size;
   using std::string::resize;
};

/* @cond PRIVATE */
inline key_type make_prefix(eosio::name table_name, eosio::name index_name, uint8_t status = 1) {
   auto bige_table = swap_endian<uint64_t>(table_name.value);
   auto bige_index = swap_endian<uint64_t>(index_name.value);

   constexpr size_t index_name_size = sizeof(index_name);
   constexpr size_t buffer_size = (2 * index_name_size) + sizeof(status);

   key_type ret;
   ret.resize(buffer_size);

   memcpy(ret.data(), &status, sizeof(status));
   memcpy(ret.data() + sizeof(status), &bige_table, index_name_size);
   memcpy(ret.data() + sizeof(status) + index_name_size, &bige_index, index_name_size);

   return ret;
}

inline key_type table_key(const key_type& prefix, const key_type& key) {
   return prefix + key;
}

template <typename I>
inline I flip_msb(I val) {
   constexpr static size_t bits = sizeof(I) * 8;
   return val ^ (static_cast<I>(1) << (bits - 1));
}

template <typename I>
inline I get_msb(I val) {
   constexpr static size_t bits = sizeof(I) * 8;
   return val >> (bits - 1);
}

template <typename I, typename std::enable_if_t<std::is_integral<I>::value, int> = 0>
inline key_type make_key(I val) {
   if (std::is_signed<I>::value) {
      val = flip_msb(val);
   }

   auto big_endian = swap_endian<I>(val);

   const char* bytes = reinterpret_cast<char*>(&big_endian);
   constexpr size_t size = sizeof(big_endian);
   key_type s(bytes, size);
   return s;
}

template <typename I, typename F>
inline key_type make_floating_key(F val) {
   if (val == -0) {
      val = +0;
   }

   auto* ival = reinterpret_cast<I*>(&val);
   I bit_val;
   auto msb = get_msb(*ival);
   if (msb) {
      // invert all the bits
      bit_val = ~(*ival);
   } else {
      // invert just msb
      bit_val = flip_msb(*ival);
   }

   auto big_endian = swap_endian<I>(bit_val);

   const char* bytes = reinterpret_cast<char*>(&big_endian);
   constexpr size_t size = sizeof(big_endian);
   key_type s(bytes, size);
   return s;
}

inline key_type make_key(float val) {
   return make_floating_key<uint32_t>(val);
}

inline key_type make_key(double val) {
   return make_floating_key<uint64_t>(val);
}

inline key_type make_key(const char* str, size_t size) {
   size_t data_size = size + 3;
   void* data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

   memcpy(data_buffer, str, size);

   ((char*)data_buffer)[data_size - 3] = 0x01;
   ((char*)data_buffer)[data_size - 2] = 0x00;
   ((char*)data_buffer)[data_size - 1] = 0x00;

   key_type s((const char*)data_buffer, data_size);

   if (data_size > detail::max_stack_buffer_size) {
      free(data_buffer);
   }
   return s;
}

inline key_type make_key(const std::string& val) {
   return make_key(val.data(), val.size());
}

inline key_type make_key(const std::string_view& val) {
   return make_key(val.data(), val.size());
}

inline key_type make_key(const char* str) {
   return make_key(std::string_view{str});
}

inline key_type make_key(eosio::name n) {
   return make_key(n.value);
}

inline key_type make_key(key_type&& val) {
   return std::move(val);
}

inline key_type make_key(const key_type& val) {
   return val;
}

template <typename S, typename std::enable_if_t<std::is_class<S>::value, int> = 0>
inline key_type make_key(const S& val) {
   size_t data_size = 0;
   size_t pos = 0;
   void* data_buffer;

   boost::pfr::for_each_field(val, [&](auto& field) {
      data_size += sizeof(field);
   });

   data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

   boost::pfr::for_each_field(val, [&](auto& field) {
      auto kt = make_key(field);
      memcpy((char*)data_buffer + pos, kt.data(), kt.size());
      pos += kt.size();
   });

   key_type s((const char*)data_buffer, data_size);

   if (data_size > detail::max_stack_buffer_size) {
      free(data_buffer);
   }
   return s;
}

template <typename... Args>
inline key_type make_key(const std::tuple<Args...>& val) {
   size_t data_size = 0;
   size_t pos = 0;
   void* data_buffer;

   boost::fusion::for_each(val, [&](auto& field) {
      data_size += sizeof(field);
   });

   data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

   boost::fusion::for_each(val, [&](auto& field) {
      auto kt = make_key(field);
      memcpy((char*)data_buffer + pos, kt.data(), kt.size());
      pos += kt.size();
   });

   key_type s((const char*)data_buffer, data_size);

   if (data_size > detail::max_stack_buffer_size) {
      free(data_buffer);
   }
   return s;
}
/* @endcond */

// This is the "best" way to document a function that does not technically exist using Doxygen.
#if EOSIO_CDT_DOXYGEN
/**
 * @brief A function for converting types to the appropriate binary representation for the EOSIO Key Value database.
 * @details The CDT provides implementations of this function for many of the common primitives and for structs/tuples.
 * If sticking with standard types, contract developers should not need to interact with this function.
 * If doing something more advanced, contract developers may need to provide their own implementation for a special type.
 */
template <typename T>
inline key_type make_key(T val) {
   return {};
}
#endif

static constexpr eosio::name kv_ram = "eosio.kvram"_n;
static constexpr eosio::name kv_disk = "eosio.kvdisk"_n;

template <typename ...Types>
using non_unique = std::tuple<Types...>;

template<typename T>
class kv_table;

namespace kv_detail {

   class kv_table_base;

   class kv_index {

   public:
      eosio::name index_name;
      eosio::name table_name;
      eosio::name contract_name;

      key_type to_table_key(const key_type& k) const { return table_key(prefix, k); }

   protected:
      kv_index() = default;

      template <typename KF, typename T>
      kv_index(eosio::name index_name, KF&& kf, T*) : index_name{index_name} {
         key_function = [=](const void* t) {
            return make_key(std::invoke(kf, static_cast<const T*>(t)));
         };
      }

      template<typename T>
      key_type get_key(const T& inst) const { return key_function(&inst); }
      key_type get_key_void(const void* ptr) const { return key_function(ptr); }

      void get(const key_type& key, void* ret_val, void (*deserialize)(void*, const void*, std::size_t)) const;

      kv_table_base* tbl;
      key_type prefix;

   private:
      template<typename T>
      friend class eosio::kv_table;
      friend class kv_table_base;
      friend class iterator_base;

      std::function<key_type(const void*)> key_function;

      virtual void setup() = 0;
   };

   class kv_table_base {
    protected:
      friend class kv_index;
      friend class iterator_base;
      eosio::name contract_name;
      eosio::name table_name;
      uint64_t db_name;

      eosio::name primary_index_name;

      kv_index* primary_index;
      std::vector<kv_index*> secondary_indices;

      void put(const void* value, void* old_value,
               std::size_t (*get_size)(const void*),
               void (*deserialize)(void*, const void*, std::size_t),
               void (*serialize)(const void*, void*, std::size_t)) {
         uint32_t value_size;

         auto primary_key = primary_index->get_key_void(value);
         auto tbl_key = table_key(make_prefix(table_name, primary_index->index_name), primary_key);

         auto primary_key_found = internal_use_do_not_use::kv_get(db_name, contract_name.value, tbl_key.data(), tbl_key.size(), value_size);

         if (primary_key_found) {
            void* buffer = value_size > detail::max_stack_buffer_size ? malloc(value_size) : alloca(value_size);
            auto copy_size = internal_use_do_not_use::kv_get_data(db_name, 0, (char*)buffer, value_size);

            deserialize(old_value, buffer, copy_size);

            if (value_size > detail::max_stack_buffer_size) {
               free(buffer);
            }
         }

         for (const auto& idx : secondary_indices) {
            uint32_t value_size;
            auto sec_tbl_key = table_key(make_prefix(table_name, idx->index_name), idx->get_key_void(value));
            auto sec_found = internal_use_do_not_use::kv_get(db_name, contract_name.value, sec_tbl_key.data(), sec_tbl_key.size(), value_size);

            if (!primary_key_found) {
               eosio::check(!sec_found, "Attempted to store an existing secondary index.");
               internal_use_do_not_use::kv_set(db_name, contract_name.value, sec_tbl_key.data(), sec_tbl_key.size(), tbl_key.data(), tbl_key.size());
            } else {
               if (sec_found) {
                  void* buffer = value_size > detail::max_stack_buffer_size ? malloc(value_size) : alloca(value_size);
                  auto copy_size = internal_use_do_not_use::kv_get_data(db_name, 0, (char*)buffer, value_size);

                  auto res = memcmp(buffer, tbl_key.data(), copy_size);
                  eosio::check(copy_size == tbl_key.size() && res == 0, "Attempted to update an existing secondary index.");

                  if (copy_size > detail::max_stack_buffer_size) {
                     free(buffer);
                  }
               } else {
                  auto old_sec_key = table_key(make_prefix(table_name, idx->index_name), idx->get_key_void(old_value));
                  internal_use_do_not_use::kv_erase(db_name, contract_name.value, old_sec_key.data(), old_sec_key.size());
                  internal_use_do_not_use::kv_set(db_name, contract_name.value, sec_tbl_key.data(), sec_tbl_key.size(), tbl_key.data(), tbl_key.size());
               }
            }
         }

         size_t data_size = get_size(value);
         void* data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

         serialize(value, data_buffer, data_size);

         internal_use_do_not_use::kv_set(db_name, contract_name.value, tbl_key.data(), tbl_key.size(), (const char*)data_buffer, data_size);

         if (data_size > detail::max_stack_buffer_size) {
            free(data_buffer);
         }
      }

      void erase(const void* value) {
         uint32_t value_size;

         auto primary_key = primary_index->get_key_void(value);
         auto tbl_key = table_key(make_prefix(table_name, primary_index->index_name), primary_key);
         auto primary_key_found = internal_use_do_not_use::kv_get(db_name, contract_name.value, tbl_key.data(), tbl_key.size(), value_size);

         if (!primary_key_found) {
            return;
         }

         for (const auto& idx : secondary_indices) {
            auto sec_tbl_key = table_key(make_prefix(table_name, idx->index_name), idx->get_key_void(value));
            internal_use_do_not_use::kv_erase(db_name, contract_name.value, sec_tbl_key.data(), sec_tbl_key.size());
         }

         internal_use_do_not_use::kv_erase(db_name, contract_name.value, tbl_key.data(), tbl_key.size());
      }
   };

   inline void kv_index::get(const key_type& key, void* ret_val, void (*deserialize)(void*, const void*, std::size_t)) const {
      uint32_t value_size;
      uint32_t actual_data_size;

      auto success = internal_use_do_not_use::kv_get(tbl->db_name, contract_name.value, key.data(), key.size(), value_size);
      if (!success) {
         return;
      }

      void* buffer = value_size > detail::max_stack_buffer_size ? malloc(value_size) : alloca(value_size);
      auto copy_size = internal_use_do_not_use::kv_get_data(tbl->db_name, 0, (char*)buffer, value_size);

      void* deserialize_buffer = buffer;
      size_t deserialize_size = copy_size;

      bool is_primary = index_name == tbl->primary_index_name;
      if (!is_primary) {
         auto success = internal_use_do_not_use::kv_get(tbl->db_name, contract_name.value, (char*)buffer, copy_size, actual_data_size);
         eosio::check(success, "failure getting primary key");

         void* pk_buffer = actual_data_size > detail::max_stack_buffer_size ? malloc(actual_data_size) : alloca(actual_data_size);
         auto pk_copy_size = internal_use_do_not_use::kv_get_data(tbl->db_name, 0, (char*)pk_buffer, actual_data_size);

         deserialize_buffer = pk_buffer;
         deserialize_size = pk_copy_size;
      }

      deserialize(ret_val, deserialize_buffer, deserialize_size);

      if (value_size > detail::max_stack_buffer_size) {
         free(buffer);
      }

      if (!is_primary && actual_data_size > detail::max_stack_buffer_size) {
         free(deserialize_buffer);
      }
   }

   class iterator_base {
   public:
      enum class status {
         iterator_ok     = 0,  // Iterator is positioned at a key-value pair
         iterator_erased = -1, // The key-value pair that the iterator used to be positioned at was erased
         iterator_end    = -2, // Iterator is out-of-bounds
      };

      iterator_base() = default;

      iterator_base(uint32_t itr, status itr_stat, const kv_index* index) : itr{itr}, itr_stat{itr_stat}, index{index} {}

      iterator_base(iterator_base&& other) :
         itr(std::exchange(other.itr, 0)),
         itr_stat(std::move(other.itr_stat))
      {}

      ~iterator_base() {
         if (itr) {
            internal_use_do_not_use::kv_it_destroy(itr);
         }
      }

      iterator_base& operator=(iterator_base&& other) {
         if (itr) {
            internal_use_do_not_use::kv_it_destroy(itr);
         }
         itr = std::exchange(other.itr, 0);
         itr_stat = std::move(other.itr_stat);
         return *this;
      }

      bool is_end()const { return itr_stat != status::iterator_end; }

      /**
       * Returns the value that the iterator points to.
       * @ingroup keyvalue
       *
       * @return The value that the iterator points to.
       */
     void value(void* val, void (*deserialize)(void*, const void*, std::size_t)) const {
         using namespace detail;

         eosio::check(itr_stat != status::iterator_end, "Cannot read end iterator");

         uint32_t value_size;
         uint32_t actual_value_size;
         uint32_t actual_data_size;
         uint32_t offset = 0;

         // call once to get the value_size
         internal_use_do_not_use::kv_it_value(itr, 0, (char*)nullptr, 0, value_size);

         void* buffer = value_size > detail::max_stack_buffer_size ? malloc(value_size) : alloca(value_size);
         auto stat = internal_use_do_not_use::kv_it_value(itr, offset, (char*)buffer, value_size, actual_value_size);

         eosio::check(static_cast<status>(stat) == status::iterator_ok, "Error reading value");

         void* deserialize_buffer = buffer;
         size_t deserialize_size = actual_value_size;

         bool is_primary = index->index_name == index->tbl->primary_index_name;
         if (!is_primary) {
            auto success = internal_use_do_not_use::kv_get(index->tbl->db_name, index->contract_name.value, (char*)buffer, actual_value_size, actual_data_size);
            eosio::check(success, "failure getting primary key in `value()`");

            void* pk_buffer = actual_data_size > detail::max_stack_buffer_size ? malloc(actual_data_size) : alloca(actual_data_size);
            internal_use_do_not_use::kv_get_data(index->tbl->db_name, 0, (char*)pk_buffer, actual_data_size);

            deserialize_buffer = pk_buffer;
            deserialize_size = actual_data_size;
         }

         deserialize(val, deserialize_buffer, deserialize_size);

         if (value_size > detail::max_stack_buffer_size) {
            free(buffer);
         }

         if (!is_primary && actual_data_size > detail::max_stack_buffer_size) {
            free(deserialize_buffer);
         }
      }

      key_type key() const {
         uint32_t actual_value_size;
         uint32_t value_size;

         // call once to get the value size
         internal_use_do_not_use::kv_it_key(itr, 0, (char*)nullptr, 0, value_size);

         void* buffer = value_size > detail::max_stack_buffer_size ? malloc(value_size) : alloca(value_size);
         auto stat = internal_use_do_not_use::kv_it_key(itr, 0, (char*)buffer, value_size, actual_value_size);

         eosio::check(static_cast<status>(stat) == status::iterator_ok, "Error getting key");

         return {(char*)buffer, actual_value_size};
      }

   protected:
      uint32_t itr;
      status itr_stat;

      const kv_index* index;

      int compare(const iterator_base& b) const {
         bool a_is_end = !itr || itr_stat == status::iterator_end;
         bool b_is_end = !b.itr || b.itr_stat == status::iterator_end;
         if (a_is_end && b_is_end) {
            return 0;
         } else if (a_is_end && b.itr) {
            return 1;
         } else if (itr && b_is_end) {
            return -1;
         } else {
            return internal_use_do_not_use::kv_it_compare(itr, b.itr);
         }
      }
   };

}

/**
 * @defgroup keyvalue Key Value Table
 * @ingroup contracts
 *
 * @brief Defines an EOSIO Key Value Table
 * @details EOSIO Key Value API provides a C++ interface to the EOSIO Key Value database.
 * Key Value Tables require 1 primary index, of any type that can be serialized to a binary representation.
 * Key Value Tables support 0 or more secondary index, of any type that can be serialized to a binary representation.
 * Indexes must be a member variable or a member function.
 *
 * @tparam T         - the type of the data stored as the value of the table
  */
template<typename T>
class kv_table : kv_detail::kv_table_base {
public:
   template<typename K>
   class index;

private:
   using kv_index = kv_detail::kv_index;

   class base_iterator : public kv_detail::iterator_base {
   public:
      using iterator_base::iterator_base;
      /**
       * Returns the value that the iterator points to.
       * @ingroup keyvalue
       *
       * @return The value that the iterator points to.
       */
      T value() const {
         T val;
         iterator_base::value(&val, &kv_table::deserialize_fun);
         return val;
      }
   };

   class iterator : public base_iterator {
      using base_iterator::itr;
      using base_iterator::itr_stat;
      using base_iterator::index;

      template<typename K>
      friend class index;

   public:
      using status = typename base_iterator::status;

      iterator() = default;

      iterator(uint32_t itr, status itr_stat, const kv_index* index) : base_iterator{itr, itr_stat, index} {}

      iterator(iterator&& other) : base_iterator{std::move(other)} {}

      iterator& operator=(iterator&& other) {
         if (itr) {
            internal_use_do_not_use::kv_it_destroy(itr);
         }
         itr = std::exchange(other.itr, 0);
         itr_stat = std::move(other.itr_stat);
         index = std::move(other.index);
         return *this;
      }

      iterator& operator++() {
         eosio::check(itr_stat != status::iterator_end, "cannot increment end iterator");
         itr_stat = static_cast<status>(internal_use_do_not_use::kv_it_next(itr));
         return *this;
      }

      iterator& operator--() {
         if (!itr) {
            itr = internal_use_do_not_use::kv_it_create(static_cast<kv_table*>(index->tbl)->db_name, index->contract_name.value, index->prefix.data(), index->prefix.size());
         }
         itr_stat = static_cast<status>(internal_use_do_not_use::kv_it_prev(itr));
         eosio::check(itr_stat != status::iterator_end, "decremented past the beginning");
         return *this;
      }

      bool operator==(const iterator& b) const {
         return base_iterator::compare(b) == 0;
      }

      bool operator!=(const iterator& b) const {
         return base_iterator::compare(b) != 0;
      }

      bool operator<(const iterator& b) const {
         return base_iterator::compare(b) < 0;
      }

      bool operator<=(const iterator& b) const {
         return base_iterator::compare(b) <= 0;
      }

      bool operator>(const iterator& b) const {
         return base_iterator::compare(b) > 0;
      }

      bool operator>=(const iterator& b) const {
         return base_iterator::compare(b) >= 0;
      }
   };

   class reverse_iterator : public base_iterator {
      using base_iterator::itr;
      using base_iterator::itr_stat;
      using base_iterator::index;

   public:
      using status = typename base_iterator::status;

      reverse_iterator() = default;

      reverse_iterator(uint32_t itr, status itr_stat, const kv_index* index) : base_iterator{itr, itr_stat, index} {}

      reverse_iterator(reverse_iterator&& other) : base_iterator{std::move(other)} {}

      reverse_iterator& operator=(reverse_iterator&& other) {
         if (itr) {
            internal_use_do_not_use::kv_it_destroy(itr);
         }
         itr = std::exchange(other.itr, 0);
         itr_stat = std::move(other.itr_stat);
         index = std::move(other.index);
         return *this;
      }

      reverse_iterator& operator++() {
         eosio::check(itr_stat != status::iterator_end, "incremented past the end");
         itr_stat = static_cast<status>(internal_use_do_not_use::kv_it_prev(itr));
         return *this;
      }

      reverse_iterator& operator--() {
         if (!itr) {
            itr = internal_use_do_not_use::kv_it_create(static_cast<kv_table*>(index->tbl)->db_name, index->contract_name.value, index->prefix.data(), index->prefix.size());
            itr_stat = static_cast<status>(internal_use_do_not_use::kv_it_lower_bound(itr, "", 0));
         }
         itr_stat = static_cast<status>(internal_use_do_not_use::kv_it_next(itr));
         eosio::check(itr_stat != status::iterator_end, "decremented past the beginning");
         return *this;
      }

      int compare(const reverse_iterator& b) const {
         bool a_is_end = !itr || itr_stat == status::iterator_end;
         bool b_is_end = !b.itr || b.itr_stat == status::iterator_end;
         if (a_is_end && b_is_end) {
            return 0;
         } else if (a_is_end && b.itr) {
            return 1;
         } else if (itr && b_is_end) {
            return -1;
         } else {
            return -(internal_use_do_not_use::kv_it_compare(itr, b.itr));
         }
      }

      bool operator==(const reverse_iterator& b) const {
         return compare(b) == 0;
      }

      bool operator!=(const reverse_iterator& b) const {
         return compare(b) != 0;
      }

      bool operator<(const reverse_iterator& b) const {
         return compare(b) < 0;
      }

      bool operator<=(const reverse_iterator& b) const {
         return compare(b) <= 0;
      }

      bool operator>(const reverse_iterator& b) const {
         return compare(b) > 0;
      }

      bool operator>=(const reverse_iterator& b) const {
         return compare(b) >= 0;
      }
   };

public:
   using iterator = kv_table::iterator;
   using value_type = T;

   /**
    * @ingroup keyvalue
    *
    * @brief Defines an index on an EOSIO Key Value Table
    * @details A Key Value Index allows a user of the table to search based on a given field.
    * The only restrictions on that field are that it is serializable to a binary representation sortable by the KV intrinsics.
    * Convenience functions exist to handle most of the primitive types as well as some more complex types, and are
    * used automatically where possible.
    *
    * @tparam K - The type of the key used in the index.
    */
   template <typename K>
   class index : public kv_index {
   public:
      using iterator = kv_table::iterator;
      using kv_table<T>::kv_index::tbl;
      using kv_table<T>::kv_index::table_name;
      using kv_table<T>::kv_index::contract_name;
      using kv_table<T>::kv_index::index_name;
      using kv_table<T>::kv_index::prefix;

      template <typename KF>
      index(eosio::name name, KF&& kf) : kv_index{name, kf, (T*)nullptr} {
         static_assert(std::is_same_v<K, std::remove_cv_t<std::decay_t<decltype(std::invoke(kf, std::declval<const T*>()))>>>,
               "Make sure the variable/function passed to the constructor returns the same type as the template parameter.");
      }

      /**
       * Search for an existing object in a table by the index, using the given key.
       * @ingroup keyvalue
       *
       * @param key - The key to search for.
       * @return An iterator to the found object OR the `end` iterator if the given key was not found.
       */
      iterator find(const K& key) const {
         auto t_key = table_key(prefix, make_key(key));

         uint32_t itr = internal_use_do_not_use::kv_it_create(static_cast<kv_table*>(tbl)->db_name, contract_name.value, prefix.data(), prefix.size());
         int32_t itr_stat = internal_use_do_not_use::kv_it_lower_bound(itr, t_key.data(), t_key.size());

         auto cmp = internal_use_do_not_use::kv_it_key_compare(itr, t_key.data(), t_key.size());

         if (cmp != 0) {
            internal_use_do_not_use::kv_it_destroy(itr);
            return end();
         }

         return {itr, static_cast<typename iterator::status>(itr_stat), this};
      }

      /**
       * Check if a given key exists in the index.
       * @ingroup keyvalue
       *
       * @param key - The key to check for.
       * @return If the key exists or not.
       */
      bool exists(const K& key) const {
         uint32_t value_size;
         auto t_key = table_key(prefix, make_key(key));

         return internal_use_do_not_use::kv_get(static_cast<kv_table*>(tbl)->db_name, contract_name.value, t_key.data(), t_key.size(), value_size);
      }

      /**
       * Get the value for an existing object in a table by the index, using the given key.
       * @ingroup keyvalue
       *
       * @param key - The key to search for.
       * @return The value corresponding to the key.
       */
      T operator[](const K& key) const {
         auto opt = get(key);
         eosio::check(opt.has_value(), "Key not found in `[]`");
         return *opt;
      }

      /**
       * Get the value for an existing object in a table by the index, using the given key.
       * @ingroup keyvalue
       *
       * @param key - The key to search for.
       * @return A std::optional of the value corresponding to the key.
       */
      std::optional<T> get(const K& key) const {
         std::optional<T> ret_val;
         auto k = table_key(prefix, make_key(key));
         kv_index::get(k, &ret_val, &deserialize_optional_fun);
         return ret_val;
      }

      /**
       * Returns an iterator to the object with the lowest key (by this index) in the table.
       * @ingroup keyvalue
       *
       * @return An iterator to the object with the lowest key (by this index) in the table.
       */
      iterator begin() const {
         uint32_t itr = internal_use_do_not_use::kv_it_create(static_cast<kv_table*>(tbl)->db_name, contract_name.value, prefix.data(), prefix.size());
         int32_t itr_stat = internal_use_do_not_use::kv_it_lower_bound(itr, "", 0);

         return {itr, static_cast<typename iterator::status>(itr_stat), this};
      }

      /**
       * Returns an iterator pointing past the end. It does not point to any element, therefore `value` should not be called on it.
       * @ingroup keyvalue
       *
       * @return An iterator pointing past the end.
       */
      iterator end() const {
         return {0, iterator::status::iterator_end, this};
      }

      /**
       * Returns a reverse iterator to the object with the highest key (by this index) in the table.
       * @ingroup keyvalue
       *
       * @return A reverse iterator to the object with the highest key (by this index) in the table.
       */
      reverse_iterator rbegin() const {
         uint32_t itr = internal_use_do_not_use::kv_it_create(static_cast<kv_table*>(tbl)->db_name, contract_name.value, prefix.data(), prefix.size());
         int32_t itr_stat = internal_use_do_not_use::kv_it_prev(itr);

         return {itr, static_cast<typename iterator::status>(itr_stat), this};
      }

      /**
       * Returns a reverse iterator pointing past the beginning. It does not point to any element, therefore `value` should not be called on it.
       * @ingroup keyvalue
       *
       * @return A reverse iterator pointing past the beginning.
       */
      reverse_iterator rend() const {
         return {0, iterator::status::iterator_end, this};
      }

      /**
       * Returns an iterator pointing to the element with the lowest key greater than or equal to the given key.
       * @ingroup keyvalue
       *
       * @return An iterator pointing to the element with the lowest key greater than or equal to the given key.
       */
      iterator lower_bound(const K& key) const {
         auto t_key = table_key(prefix, make_key(key));

         uint32_t itr = internal_use_do_not_use::kv_it_create(static_cast<kv_table*>(tbl)->db_name, contract_name.value, prefix.data(), prefix.size());
         int32_t itr_stat = internal_use_do_not_use::kv_it_lower_bound(itr, t_key.data(), t_key.size());

         return {itr, static_cast<typename iterator::status>(itr_stat), this};
      }

      /**
       * Returns an iterator pointing to the first element greater than the given key.
       * @ingroup keyvalue
       *
       * @return An iterator pointing to the first element greater than the given key.
       */
      iterator upper_bound(const K& key) const {
         auto t_key = table_key(prefix, make_key(key));
         auto it = lower_bound(key);

         int32_t cmp;

         if (it.itr == 0 || it.itr_stat == base_iterator::status::iterator_end) {
            cmp = 1;
         } else {
            cmp = internal_use_do_not_use::kv_it_key_compare(it.itr, t_key.data(), t_key.size());
         }
         if (cmp == 0) {
            ++it;
         }

         return it;
      }

      /**
       * Returns a vector of objects that fall between the specifed range. The range is inclusive, exclusive.
       * @ingroup keyvalue
       *
       * @param begin - The beginning of the range (inclusive).
       * @param end - The end of the range (exclusive).
       * @return A vector containing all the objects that fall between the range.
       */
      std::vector<T> range(const K& b, const K& e) const {
         std::vector<T> return_values;

         for(auto itr = lower_bound(b), end_itr = lower_bound(e); itr < end_itr; ++itr) {
            return_values.push_back(itr.value());
         }

         return return_values;
      }

      void setup() override {
         prefix = make_prefix(table_name, index_name);
      }
   };

   /**
    * @ingroup keyvalue
    * Puts a value into the table. If the value already exists, it updates the existing entry.
    * The key is determined from the defined primary index.
    * If the put attempts to store over an existing secondary index, the transaction will be aborted.
    *
    * @param value - The entry to be stored in the table.
    */
   void put(const T& value) {
      uint32_t value_size;
      T old_value;
      kv_table_base::put(&value, &old_value, &get_size_fun, &deserialize_fun, &serialize_fun);
   }

   static void deserialize_optional_fun(void* value, const void* buffer, std::size_t buffer_size) {
      static_cast<std::optional<T>*>(value)->emplace();
      return detail::deserialize(**static_cast<std::optional<T>*>(value), buffer, buffer_size);
   }
   static void deserialize_fun(void* value, const void* buffer, std::size_t buffer_size) {
      return detail::deserialize(*static_cast<T*>(value), buffer, buffer_size);
   }
   static void serialize_fun(const void* value, void* buffer, std::size_t buffer_size) {
      return detail::serialize(*static_cast<const T*>(value), buffer, buffer_size);
   }
   static std::size_t get_size_fun(const void* value) {
      return detail::get_size(*static_cast<const T*>(value));
   }

   /**
    * Removes a value from the table.
    * @ingroup keyvalue
    *
    * @param key - The key of the value to be removed.
    */
   void erase(const T& value) {
      kv_table_base::erase(&value);
   }

protected:
   kv_table() = default;

   template <typename I>
   void setup_indices(I& index) {
      kv_index* idx = &index;
      idx->contract_name = contract_name;
      idx->table_name = table_name;
      idx->tbl = this;

      idx->setup();
      secondary_indices.push_back(idx);
   }

   template <typename PrimaryIndex, typename... SecondaryIndices>
   void init(eosio::name contract, eosio::name table, eosio::name db, PrimaryIndex& prim_index, SecondaryIndices&... indices) {
      validate_types(prim_index);
      (validate_types(indices), ...);

      contract_name = contract;
      table_name = table;
      db_name = db.value;

      primary_index = &prim_index;
      primary_index->contract_name = contract_name;
      primary_index->table_name = table_name;
      primary_index->tbl = this;

      primary_index->setup();

      primary_index_name = primary_index->index_name;

      (setup_indices(indices), ...);
   }

private:

   constexpr void validate_types() {}

   template <typename Type>
   constexpr void validate_types(Type& t) {
      constexpr bool is_kv_index = std::is_base_of_v<kv_index, std::decay_t<Type>>;
      static_assert(is_kv_index, "Incorrect type passed to init. Must be a reference to an index.");
   }
};

template <typename T, eosio::name::raw SingletonName, eosio::name::raw DbName = "eosio.kvram"_n>
class kv_singleton {
   struct state {
      T value;
      char* raw_original;
      size_t raw_original_size;

      bool is_dirty = false;
      bool is_cached = false;

      ~state() {
         if (raw_original_size) {
            free(raw_original);
         }
      }
   };

public:
   explicit kv_singleton(eosio::name contract_name) : contract_name{contract_name} {
      key = make_prefix();
   }

   ~kv_singleton() {
      if (get_state().is_dirty) {
         store();
      }
   }

   const T& get_or_create() {
       if( !exists() ) {
           auto& ste = get_state();
           ste.is_cached = true;
           ste.is_dirty = true;
       }
       return get();
   };

   T& create_or_modify() {
       if( !exists() ) {
           auto& ste = get_state();
           ste.is_cached = true;
           ste.is_dirty = true;
       }
       return modify();
   };

   const T& get() {
      auto& ste = get_state();
      load_state(ste);

      return ste.value;
   }

   T& modify() {
      auto& ste = get_state();
      load_state(ste);
      ste.is_dirty = true;

      return ste.value;
   }

   void set(const T& val) {
      auto& ste = get_state();
      ste.value = val;
      ste.is_dirty = true;
      ste.is_cached = true;
   }

   void set(T&& val) {
      auto& ste = get_state();
      ste.value = val;
      ste.is_dirty = true;
      ste.is_cached = true;
   }

   bool exists() const {
      uint32_t value_size;

      return internal_use_do_not_use::kv_get(db_name, contract_name.value, key.data(), key.size(), value_size);
   }

   void erase() {
      internal_use_do_not_use::kv_erase(db_name, contract_name.value, key.data(), key.size());
      auto& ste = get_state();
      ste.is_cached = false;
      ste.is_dirty = false;
      ste.raw_original_size = 0;
      free(ste.raw_original);
   }

   void store() {
      auto& ste = get_state();
      if (ste.is_dirty) {
         size_t data_size = detail::get_size(ste.value);
         void* data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

         detail::serialize(ste.value, data_buffer, data_size);

         if (ste.raw_original_size != data_size || memcmp(ste.raw_original, data_buffer, data_size) != 0) {
            internal_use_do_not_use::kv_set(db_name, contract_name.value, key.data(), key.size(), (const char*)data_buffer, data_size);
         }
         ste.is_dirty = false;
      }
   }

private:
   constexpr static uint64_t db_name = static_cast<uint64_t>(DbName);
   constexpr static uint64_t singleton_name = static_cast<uint64_t>(SingletonName);

   eosio::name contract_name;
   key_type key;

   key_type make_prefix() {
      return make_key(std::make_tuple(0x02, singleton_name));
   }

   state& get_state() {
      static state value;
      return value;
   }

   const state& get_state() const {
      static state value;
      return value;
   }

   void load_state(state& ste) {
      if (!ste.is_cached) {
         uint32_t copy_size;
         uint32_t value_size;

         auto success = internal_use_do_not_use::kv_get(db_name, contract_name.value, key.data(), key.size(), value_size);

         if( !success ) {
             eosio::check(success, "the singleton '" + eosio::name(SingletonName).to_string() + "' does not exist");
         }

         ste.raw_original = (char*)malloc(value_size);
         ste.raw_original_size = value_size;
         copy_size = internal_use_do_not_use::kv_get_data(db_name, 0, ste.raw_original, value_size);

         detail::deserialize(ste.value, ste.raw_original, copy_size);
         ste.is_cached = true;
      }
   }
};
} // eosio
