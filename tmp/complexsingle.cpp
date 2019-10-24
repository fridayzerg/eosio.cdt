#include <eosio/eosio.hpp>
#include <eosio/tester.hpp>

using namespace eosio;
using namespace eosio::native;

int testnum = 0;
int primary_key = 0;

class [[eosio::contract]] complexsingle : public contract {
   public:
      using contract::contract;

      complexsingle(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

      [[eosio::action]]
         void insert(uint64_t pk, name user) {
            eosio::print_f("INSERT\n");
            address_index addresses( get_self(), get_first_receiver().value );
            auto itr = addresses.find(pk);
            check(itr == addresses.end(), "This record already exists");
            addresses.emplace(get_self(), [&]( auto& row ) {
               row.key = pk;
               row.user = user;
            });
         }

      [[eosio::action]]
         void modify(uint64_t pk, name user) {
            eosio::print_f("MODIFY\n");
            address_index addresses( get_self(), get_first_receiver().value );
            auto itr = addresses.find(pk);
            check(itr != addresses.end(), "This record does not exist");
            addresses.modify(itr, get_self(), [&]( auto& row ) {
               row.user = user;
            });
         }

      [[eosio::action]]
         void erase(uint64_t pk) {
            eosio::print_f("ERASE\n");
            address_index addresses( get_self(), get_first_receiver().value);

            auto itr = addresses.find(pk);
            check(itr != addresses.end(), "Record does not exist but should");
            addresses.erase(itr);
            itr = addresses.find(pk);
            check(itr == addresses.end(), "Record exists when it shouldn't");
         }

      [[eosio::action]]
         void move(uint64_t pk) {
            eosio::print_f("MOVE\n");
            address_index addresses( get_self(), get_first_receiver().value);

            auto itr = addresses.begin();
            itr++;
            itr++;
            itr--;
         }

   private:
      struct [[eosio::table]] person {
         uint64_t key;
         name user;
         uint64_t primary_key() const { return key; }
      };
      typedef eosio::multi_index<"people"_n, person> address_index;

};

EOSIO_DISPATCH(complexsingle, (insert)(modify)(erase)(move))

EOSIO_TEST_BEGIN(complexsingle_test)
   intrinsics::set_intrinsic<intrinsics::read_action_data>(
      [](void* m, uint32_t len) {
         check(len <= 2*sizeof(eosio::name), "failed from read_action_data");
         uint64_t pk = primary_key;
         uint64_t user;

         switch (testnum) {
         case 0:
            user = eosio::name("bob").value;
            break;
         case 1:
            user = eosio::name("alice").value;
            break;
         case 2:
            user = eosio::name("john").value;
            break;
         case 3:
            user = eosio::name("lucy").value;
            break;
         }

         memcpy(m, &pk, sizeof(uint64_t));
         memcpy((char*)m + sizeof(eosio::name), &user, sizeof(eosio::name));
         return len; 
      }
   );

   intrinsics::set_intrinsic<intrinsics::action_data_size>(
      []() {
         return (uint32_t)2*sizeof(eosio::name);
      }
   );

   intrinsics::set_intrinsic<intrinsics::current_receiver>(
      []() {
         return eosio::name{"test"}.value;
      }
   );


   testnum = 0;
   primary_key = 0;
   apply("test"_n.value, "test"_n.value, "insert"_n.value);

   testnum = 1;
   primary_key = 1;
   apply("test"_n.value, "test"_n.value, "insert"_n.value);

   testnum = 2;
   primary_key = 2;
   apply("test"_n.value, "test"_n.value, "insert"_n.value);

   testnum = 3;
   primary_key = 1;
   apply("test"_n.value, "test"_n.value, "modify"_n.value);

   primary_key = 1;
   apply("test"_n.value, "test"_n.value, "move"_n.value);

   primary_key = 1;
   apply("test"_n.value, "test"_n.value, "erase"_n.value);

EOSIO_TEST_END

int main(int argc, char** argv) {
   silence_output(false);
   EOSIO_TEST(complexsingle_test);
   return has_failed();
}
