#pragma once
#include "../../core/eosio/datastream.hpp"
#include "../../core/eosio/name.hpp"
#include "../../core/eosio/print.hpp"
#include "../../core/eosio/utility.hpp"

#include <algorithm>
#include <cctype>
#include <functional>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/pfr.hpp>

#define EOSIO_CDT_KV_INDEX_nullptr

#define EOSIO_CDT_KV_INDEX_TEST() 1
#define EOSIO_CDT_KV_INDEX_TEST_EOSIO_CDT_KV_INDEX_TEST 0,
#define EOSIO_CDT_KV_INDEX_TEST_1 1, ignore
#define EOSIO_CDT_EXPAND(x) x
#define EOSIO_CDT_CAT2(x, y) x ## y
#define EOSIO_CDT_CAT(x, y) EOSIO_CDT_CAT2(x, y)
#define EOSIO_CDT_APPLY(f, args) f args
#define EOSIO_CDT_FIX_KV_INDEX_TYPE_0(index_name) kv_index
#define EOSIO_CDT_FIX_KV_INDEX_TYPE_1(index_name) null_kv_index
#define EOSIO_CDT_FIX_KV_INDEX_TYPE(iskeyword, garbage) EOSIO_CDT_FIX_KV_INDEX_TYPE_ ## iskeyword

#define EOSIO_CDT_FIX_KV_INDEX_CONSTRUCT_0(value_class, index_name) {&value_class::index_name}
#define EOSIO_CDT_FIX_KV_INDEX_CONSTRUCT_1(value_class, index_name)
#define EOSIO_CDT_FIX_KV_INDEX_CONSTRUCT(iskeyword, garbage) EOSIO_CDT_FIX_KV_INDEX_CONSTRUCT_ ## iskeyword

#define EOSIO_CDT_KV_INDEX_TYPE(index_name)                                                                            \
   EOSIO_CDT_APPLY(EOSIO_CDT_FIX_KV_INDEX_TYPE,                                                                        \
      (EOSIO_CDT_CAT(EOSIO_CDT_KV_INDEX_TEST_,                                                                         \
           EOSIO_CDT_EXPAND(EOSIO_CDT_KV_INDEX_TEST EOSIO_CDT_KV_INDEX_ ## index_name ()))))(index_name)

#define EOSIO_CDT_KV_INDEX_CONSTRUCT(value_class, index_name)                                                          \
   EOSIO_CDT_APPLY(EOSIO_CDT_FIX_KV_INDEX_CONSTRUCT,                                                                   \
      (EOSIO_CDT_CAT(EOSIO_CDT_KV_INDEX_TEST_,                                                                         \
           EOSIO_CDT_EXPAND(EOSIO_CDT_KV_INDEX_TEST EOSIO_CDT_KV_INDEX_ ## index_name ()))))(value_class, index_name)


#define CREATE_KV_INDEX(r, value_class, index_name)                                                                    \
   EOSIO_CDT_KV_INDEX_TYPE(index_name) index_name EOSIO_CDT_KV_INDEX_CONSTRUCT(value_class, index_name);

#define LIST_INDICES(value_class, ...)                                                                                 \
   BOOST_PP_SEQ_FOR_EACH(CREATE_KV_INDEX, value_class, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define TABLE_INHERITANCE(table_class, value_class, table_name, db_name)                                               \
   eosio::kv_table<table_class, value_class, table_name##_n, db_name##_n>

#define DEFINE_TABLE(table_class, value_class, table_name, db_name, /*indices*/...)                                    \
   struct table_class : TABLE_INHERITANCE(table_class, value_class, table_name, db_name) {                             \
      struct {                                                                                                         \
         LIST_INDICES(value_class, __VA_ARGS__)                                                                        \
      } index;                                                                                                         \
                                                                                                                       \
      table_class(eosio::name contract_name) {                                                                         \
         init(contract_name, &index);                                                                                  \
      }                                                                                                                \
   };

namespace eosio {
   namespace internal_use_do_not_use {
      extern "C" {
         __attribute__((eosio_wasm_import))
         void kv_erase(uint64_t db, uint64_t contract, const char* key, uint32_t key_size);

         __attribute__((eosio_wasm_import))
         void kv_set(uint64_t db, uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size);

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
         int32_t kv_it_next(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_prev(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_lower_bound(uint32_t itr, const char* key, uint32_t size);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_key(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_value(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
      }
   }

struct key_type {
   size_t size;
   std::string buffer;

   bool operator==(const key_type& k) const {
      return std::tie(size, buffer) == std::tie(k.size, k.buffer);
   }
   bool operator!=(const key_type& k) const {
      return !(*this == k);
   }
};

namespace detail {
   constexpr static size_t max_stack_buffer_size = 512;
}

inline key_type make_prefix(eosio::name table_name, eosio::name index_name, uint8_t status = 1) {
   using namespace detail;

   auto bige_table = swap_endian<uint64_t>(table_name.value);
   auto bige_index = swap_endian<uint64_t>(index_name.value);

   size_t size_64 = sizeof(index_name);

   size_t buffer_size = (2 * size_64) + sizeof(status);
   void* buffer = buffer_size > detail::max_stack_buffer_size ? malloc(buffer_size) : alloca(buffer_size);

   memcpy(buffer, &status, sizeof(status));
   memcpy(((char*)buffer) + sizeof(status), &bige_table, size_64);
   memcpy(((char*)buffer) + sizeof(status) + size_64, &bige_index, size_64);

   std::string s((char*)buffer, buffer_size);

   if (buffer_size > detail::max_stack_buffer_size) {
      free(buffer);
   }

   return {buffer_size, s};
}

inline key_type table_key(const key_type& prefix, const key_type& key) {
   using namespace detail;

   size_t buffer_size = key.size + prefix.size;
   void* buffer = buffer_size > detail::max_stack_buffer_size ? malloc(buffer_size) : alloca(buffer_size);

   memcpy(buffer, prefix.buffer.data(), prefix.size);
   memcpy(((char*)buffer) + prefix.size, key.buffer.data(), key.size);

   std::string s((char*)buffer, buffer_size);

   if (buffer_size > detail::max_stack_buffer_size) {
      free(buffer);
   }

   return {buffer_size, s};
}

template <typename I>
inline I flip_msb(I val) {
   constexpr static size_t bits = sizeof(I) * 8;
   return val ^ (static_cast<I>(1) << (bits - 1));
}

template <typename I>
inline I get_msb(I val) {
   constexpr static size_t bits = sizeof(I) * 8;
   constexpr static I mask = static_cast<I>(0x08) << (bits - 4);
   I masked = val & mask;
   return masked >> (bits - 1);
}

template <typename I, typename std::enable_if_t<std::is_integral<I>::value, int> = 0>
inline key_type make_key(I val) {
   using namespace detail;

   if (std::is_signed<I>::value) {
      val = flip_msb(val);
   }

   auto big_endian = swap_endian<I>(val);

   size_t data_size = pack_size(big_endian);
   void* data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

   datastream<char*> data_ds((char*)data_buffer, data_size);
   data_ds << big_endian;

   std::string s((char*)data_buffer, data_size);

   if (data_size > detail::max_stack_buffer_size) {
      free(data_buffer);
   }
   return {data_size, s};
}

template <typename I, typename F>
inline key_type make_floating_key(F val) {
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

   char* bytes = reinterpret_cast<char*>(&big_endian);
   constexpr size_t size = sizeof(big_endian);
   std::string s(bytes, size);
   return {size, s};
}

template <typename F, typename std::enable_if_t<std::is_floating_point<F>::value, int> = 0>
inline key_type make_key(F val) {
   static_assert(sizeof(F) != sizeof(long double), "long doubles are currently not supported by make_key");

   if (val == -0) {
      val = +0;
   }

   if (sizeof(F) == sizeof(float)) {
      return make_floating_key<uint32_t>(val);
   }
   else {
      return make_floating_key<uint64_t>(val);
   }
}

inline key_type make_key(const char* str, size_t size, bool case_insensitive=false) {
   using namespace detail;

   size_t data_size = size + 3;
   void* data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

   if (case_insensitive) {
      std::transform(str, str + size, (char*)data_buffer, [](unsigned char c) -> unsigned char { return std::toupper(c); });
   } else {
      memcpy(data_buffer, str, size);
   }

   ((char*)data_buffer)[data_size - 3] = 0x01;
   ((char*)data_buffer)[data_size - 2] = 0x00;
   ((char*)data_buffer)[data_size - 1] = 0x00;

   std::string s((char*)data_buffer, data_size);

   if (data_size > detail::max_stack_buffer_size) {
      free(data_buffer);
   }
   return {data_size, s};
}

inline key_type make_key(const std::string& val, bool case_insensitive=false) {
   return make_key(val.data(), val.size(), case_insensitive);
}

inline key_type make_key(const char* str, bool case_insensitive=false) {
   return make_key(std::string{str}, case_insensitive);
}

inline key_type make_key(eosio::name n) {
   return make_key(n.value);
}

template <typename S, typename std::enable_if_t<std::is_class<S>::value, int> = 0>
inline key_type make_key(S val) {
   size_t data_size = 0;
   size_t pos = 0;
   void* data_buffer;

   boost::pfr::for_each_field(val, [&](auto& field) {
      data_size += pack_size(field);
   });

   data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

   boost::pfr::for_each_field(val, [&](auto& field) {
      auto kt = make_key(field);
      memcpy((char*)data_buffer + pos, kt.buffer.data(), kt.size);
      pos += kt.size;
   });

   std::string s((char*)data_buffer, data_size);

   if (data_size > detail::max_stack_buffer_size) {
      free(data_buffer);
   }
   return {data_size, s};
}

template <typename... Args>
inline key_type make_key(std::tuple<Args...> val) {
   size_t data_size = 0;
   size_t pos = 0;
   void* data_buffer;

   boost::fusion::for_each(val, [&](auto& field) {
      data_size += pack_size(field);
   });

   data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);

   boost::fusion::for_each(val, [&](auto& field) {
      auto kt = make_key(field);
      memcpy((char*)data_buffer + pos, kt.buffer.data(), kt.size);
      pos += kt.size;
   });

   std::string s((char*)data_buffer, data_size);

   if (data_size > detail::max_stack_buffer_size) {
      free(data_buffer);
   }
   return {data_size, s};
}

template<typename Class, typename T, eosio::name::raw TableName, eosio::name::raw DbName = eosio::name{"eosio.kvram"}>
class kv_table {

   enum class kv_it_stat {
      iterator_ok     = 0,  // Iterator is positioned at a key-value pair
      iterator_erased = -1, // The key-value pair that the iterator used to be positioned at was erased
      iterator_end    = -2, // Iterator is out-of-bounds
   };

public:
   class kv_index {
      class iterator {
      public:
         iterator(eosio::name contract_name, uint32_t itr, kv_it_stat itr_stat, size_t data_size, kv_index* idx) :
                  contract_name{contract_name}, itr{itr}, itr_stat{itr_stat}, data_size{data_size}, idx{idx} {}

         T value() const {
            using namespace detail;

            eosio::check(itr_stat != kv_it_stat::iterator_end, "Cannot read end iterator");
            eosio::check(data_size > 0, "Cannot read a value of size 0");

            uint32_t actual_value_size;
            uint32_t offset = 0;

            void* buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);
            auto stat = internal_use_do_not_use::kv_it_value(itr, offset, (char*)buffer, data_size, actual_value_size);

            eosio::check(static_cast<kv_it_stat>(stat) != kv_it_stat::iterator_end, "Error reading value");

            if (idx->name == idx->tbl->primary_index->name) {
               datastream<const char*> ds((char*)buffer, actual_value_size);

               T val;
               ds >> val;
               if (data_size > detail::max_stack_buffer_size) {
                  free(buffer);
               }
               return val;
            } else {
               uint32_t actual_data_size;
               auto success = internal_use_do_not_use::kv_get(db, contract_name.value, (const char*)buffer, actual_value_size, actual_data_size);
               eosio::check(success, "failure getting primary key");

               void* pk_buffer = actual_data_size > detail::max_stack_buffer_size ? malloc(actual_data_size) : alloca(actual_data_size);
               auto copy_size = internal_use_do_not_use::kv_get_data(db, 0, (char*)pk_buffer, actual_data_size);

               eosio::check(copy_size > 0, "failure getting primary index data");

               datastream<const char*> ds((char*)pk_buffer, copy_size);

               T val;
               ds >> val;

               if (actual_data_size > detail::max_stack_buffer_size) {
                  free(pk_buffer);
               }
               if (data_size > detail::max_stack_buffer_size) {
                  free(buffer);
               }
               return val;
            }
         }

         iterator& operator++() {
            eosio::check(itr_stat != kv_it_stat::iterator_end, "cannot increment end iterator");
            itr_stat = static_cast<kv_it_stat>(internal_use_do_not_use::kv_it_next(itr));
            return *this;
         }

         iterator& operator--() {
            itr_stat = static_cast<kv_it_stat>(internal_use_do_not_use::kv_it_prev(itr));
            eosio::check(itr_stat != kv_it_stat::iterator_end, "incremented past the beginning");
            return *this;
         }

         bool operator==(const iterator& b) const {
            if (itr_stat == kv_it_stat::iterator_end) {
               return b.itr_stat == kv_it_stat::iterator_end;
            }
            if (b.itr_stat == kv_it_stat::iterator_end) {
               return false;
            }
            return key() == b.key();
         }

         bool operator!=(const iterator& b) const {
            return !(*this == b);
         }

         bool operator<(const iterator& b) const {
            return itr < b.itr;
         }

      private:
         eosio::name contract_name;
         const kv_index* idx;

         size_t data_size;

         uint32_t itr;
         kv_it_stat itr_stat;

         key_type key() const {
            return idx->get_key(value());
         }
      };

   public:
      eosio::name name;
      eosio::name table_name;
      eosio::name contract_name;

      kv_table* tbl;

      kv_index() = default;

      template <typename KF>
      kv_index(KF T::*key_field) {
         key_field_function = [=](const T& t) {
            return make_key(std::invoke(key_field, t));
         };
      }

      kv_index(key_type (T::*key_function)() const): key_function{key_function} {}

      template <typename K>
      iterator find(K key) {
         uint32_t value_size;

         auto prefix = make_prefix(table_name, name);
         auto t_key = table_key(prefix, make_key(key));

         auto success = internal_use_do_not_use::kv_get(db, contract_name.value, t_key.buffer.data(), t_key.size, value_size);
         if (!success) {
            return end();
         }

         uint32_t itr = internal_use_do_not_use::kv_it_create(db, contract_name.value, prefix.buffer.data(), prefix.size);

         int32_t itr_stat = internal_use_do_not_use::kv_it_lower_bound(itr, t_key.buffer.data(), t_key.size);
         auto cmp = internal_use_do_not_use::kv_it_key_compare(itr, t_key.buffer.data(), t_key.size);

         eosio::check(cmp == 0, "This key does not exist in this iterator");

         return {contract_name, itr, static_cast<kv_it_stat>(itr_stat), value_size, this};
      }

      iterator end() {
         auto prefix = make_prefix(table_name, name);
         uint32_t itr = internal_use_do_not_use::kv_it_create(db, contract_name.value, prefix.buffer.data(), prefix.size);
         int32_t itr_stat = internal_use_do_not_use::kv_it_move_to_end(itr);

         return {contract_name, itr, static_cast<kv_it_stat>(itr_stat), 0, this};
      }

      iterator begin() {
         using namespace detail;

         auto prefix = make_prefix(table_name, name);
         uint32_t itr = internal_use_do_not_use::kv_it_create(db, contract_name.value, prefix.buffer.data(), prefix.size);
         int32_t itr_stat = internal_use_do_not_use::kv_it_lower_bound(itr, "", 0);

         uint32_t value_size;
         char* buffer;
         internal_use_do_not_use::kv_it_value(itr, 0, buffer, 0, value_size);

         return {contract_name, itr, static_cast<kv_it_stat>(itr_stat), value_size, this};
      }

      template <typename K>
      std::vector<T> range(K begin, K end) {
         eosio::check(begin <= end, "Beginning of range should be less than or equal to end");

         if (begin == end) {
            std::vector<T> t;
            t.push_back(find(begin).value());
            return t;
         }

         auto begin_itr = find(begin);
         auto end_itr = find(end);
         eosio::check(begin_itr != this->end(), "beginning of range is not in table");
         eosio::check(end_itr != this->end(), "end of range is not in table");

         std::vector<T> return_values;

         return_values.push_back(begin_itr.value());

         iterator itr = begin_itr;
         while(itr != end_itr) {
            ++itr;
            return_values.push_back(itr.value());
         }

         return return_values;
      }

      key_type get_key(const T& t) const {
         if (key_function) {
            return std::invoke(key_function, t);
         } else {
            return key_field_function(t);
         }
      }

   private:
      key_type (T::*key_function)() const = nullptr;
      std::function<key_type(T)> key_field_function;
   };

   template <typename Indices>
   void init(eosio::name contract, Indices indices) {
      contract_name = contract;
      uint64_t index_name = 1;

      auto& primary = get<0>(*indices);

      primary_index = &primary;
      primary_index->name = eosio::name{index_name};
      primary_index->contract_name = contract_name;
      primary_index->table_name = table_name;
      primary_index->tbl = this;

      ++index_name;

      for_each_field(*indices, [&](auto& idx) {
         if (idx.name != primary.name) {
            kv_index* si = &idx;
            si->name = eosio::name{index_name};
            si->contract_name = contract_name;
            si->table_name = table_name;
            si->tbl = this;
            secondary_indices.push_back(si);
            ++index_name;
         }
      });

      // Makes sure the indexes are run in the correct order.
      // This is mainly useful for debugging, this probably could be deleted.
      std::reverse(std::begin(secondary_indices), std::end(secondary_indices));
   }

   void put(const T& value) {
      using namespace detail;

      auto t_key = table_key(make_prefix(table_name, primary_index->name), primary_index->get_key(value));

      size_t data_size = pack_size(value);
      void* data_buffer = data_size > detail::max_stack_buffer_size ? malloc(data_size) : alloca(data_size);
      datastream<char*> data_ds((char*)data_buffer, data_size);
      data_ds << value;

      internal_use_do_not_use::kv_set(db, contract_name.value, t_key.buffer.data(), t_key.size, (const char*)data_buffer, data_size);

      for (auto& idx : secondary_indices) {
         auto st_key = table_key(make_prefix(table_name, idx->name), idx->get_key(value));
         internal_use_do_not_use::kv_set(db, contract_name.value, st_key.buffer.data(), st_key.size, t_key.buffer.data(), t_key.size);
      }
      
      if (data_size > detail::max_stack_buffer_size) {
         free(data_buffer);
      }
   }

   template <typename K>
   void erase(K key) {
      T val = primary_index->find(key).value();

      auto k = table_key(make_prefix(table_name, primary_index->name), make_key(key));
      internal_use_do_not_use::kv_erase(db, contract_name.value, (const char*)k.buffer.data(), k.size);

      for (auto& idx : secondary_indices) {
         auto skey = table_key(make_prefix(table_name, idx->name), idx->get_key(val));
         internal_use_do_not_use::kv_erase(db, contract_name.value, (const char*)skey.buffer.data(), skey.size);
      }
   }

private:
   constexpr static uint64_t db = static_cast<uint64_t>(DbName);
   constexpr static eosio::name table_name = static_cast<eosio::name>(TableName);

   eosio::name contract_name;

   kv_index* primary_index;
   std::vector<kv_index*> secondary_indices;

   template <size_t I, typename U>
   constexpr static auto& get(U& u) {
      constexpr size_t kv_index_size = sizeof(kv_index);
      static_assert(sizeof(U) % kv_index_size == 0);
      kv_index* indices = (kv_index*)(&u);
      return indices[I];
   }

   template <size_t S, typename U, typename F>
   constexpr static void for_each_field(U& u, F&& f) {
      f(get<S>(u));
      if constexpr (S <= 0) {
         return;
      } else {
         for_each_field<S-1>(u, f);
      }
   }

   template <typename U, typename F>
   constexpr static void for_each_field(U& u, F&& f) {
      constexpr size_t kv_index_size = sizeof(kv_index);
      static_assert(sizeof(U) % kv_index_size == 0);
      constexpr size_t num_elems = (sizeof(U) / sizeof(kv_index)) - 1;
      for_each_field<num_elems>(u, f);
   }
};
} // eosio
