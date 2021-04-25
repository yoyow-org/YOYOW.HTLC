#include <htlc.hpp>


template<typename CharT>
static std::string to_hex(const CharT* d, uint32_t s) {
  std::string r;
  const char* to_hex="0123456789abcdef";
  uint8_t* c = (uint8_t*)d;
  for( uint32_t i = 0; i < s; ++i ) {
    (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
  }
  return r;
}


void htlc::transfer_happened(const uint64_t& from, const uint64_t& to,const uint64_t& asset_id,const uint64_t&	quantity, const std::string& memo )
{
   balances_index balances(_self, from);
   auto itr = balances.find(asset_id);
   if (itr == balances.end())
   {
      // add the new account
      balances.emplace(_self, [&](auto& row)
      {
      	row.asset_id = asset_id;
        row.amount = quantity;
      });
   } 
   else
   {
      balances.modify(*itr, _self, [&](auto& row)
      {
         row.amount += quantity;
      });
   } 
}

void htlc::build(const uint64_t& sender, const uint64_t& receiver, const uint64_t& asset_id, const uint64_t& quantity,const checksum256& hashlock, const uint64_t& timelock)
{
	 graphene_assert( get_trx_sender() == sender,"invalid authority" );
   htlc_index htlcs(_self, _self);
   htlccon htlc(sender, receiver, asset_id,quantity,hashlock, timelock);
   
   graphene_assert( !contract_exists(htlc.id), "Another HTLC generates that hash. Try changing parameters slightly.");
   // make sure the receiver exists
   char toname[32];	
	 graphene_assert( get_account_name_by_id(toname,32,receiver) != -1, "receiver account does not exist");

   // make sure the sender has the funds
   graphene_assert(withdraw_balance(sender, asset_id,quantity), "Insufficient Funds");
   // build the record
   uint64_t key = htlcs.available_primary_key();
   htlcs.emplace(_self, [&](auto& row) {
      row.key = key;
      row.id = htlc.id;
      row.sender = htlc.sender;
      row.receiver = htlc.receiver;
      row.asset_id = htlc.asset_id;
      row.quantity = htlc.quantity;
      row.hashlock = htlc.hashlock;
      row.timelock = htlc.timelock;
      row.funded = true;
      row.withdrawn = htlc.withdrawn;
      row.refunded = htlc.refunded;
      row.preimage = htlc.preimage;
   });
}

void htlc::withdrawhtlc(uint64_t id, std::string preimage)
{
   htlc_index htlcs(_self, _self);
   auto iterator = htlcs.find(id);
   // basic checks
   graphene_assert( iterator != htlcs.end(), "HTLC not found");
   htlccon contract = *iterator;
   graphene_assert( !contract.withdrawn, "Tokens from this HTLC have already been withdrawn" );
   graphene_assert( !contract.refunded, "Tokens from this HTLC have already been refunded");
   graphene_assert( contract.timelock > get_head_block_time(), "HTLC timelock expired");
   checksum256 passed_in_hash;
   sha256(preimage.c_str(), preimage.length(),&passed_in_hash);
   graphene_assert( contract.hashlock == passed_in_hash, "Preimage mismatch");
   // update the contract
   htlcs.modify(iterator, _self, [&](auto& row)
	{
		row.withdrawn = true;
		row.preimage = preimage;
	});
   
   sub_balance( htlc_account,contract.asset_id,contract.quantity );
	 add_balance( contract.receiver,contract.asset_id,contract.quantity, get_trx_sender() );
 }

void htlc::refundhtlc(uint64_t id)
{
   htlc_index htlcs(_self, _self);
   auto iterator = htlcs.find(id);
   // basic checks
   graphene_assert( iterator != htlcs.end(), "HTLC not found");
   htlccon contract = *iterator;
   graphene_assert( contract.funded, "This HTLC has yet to be funded");
   graphene_assert( !contract.withdrawn, "Tokens from this HTLC have already been withdrawn" );
   graphene_assert( !contract.refunded, "Tokens from this HTLC have already been refunded");
   graphene_assert( contract.timelock <= get_head_block_time(), "HTLC timelock has not expired");
   // update the contract
   htlcs.modify(iterator, _self, [&](auto& row)
      {
         row.refunded = true;
      });
      
	sub_balance( htlc_account,contract.asset_id,contract.quantity );
	add_balance( contract.sender,contract.asset_id,contract.quantity, get_trx_sender() );
}

bool htlc::contract_exists(const checksum256& id)
{
   htlc_index htlcs(_self, _self);
   auto id_index = htlcs.get_index<N(id)>();
   
   uint64_t uid = checksum256_to_uint(id);
   auto iterator = id_index.find(uid);
   if ( iterator == id_index.end() )
      return false;

   return true;
}

bool htlc::withdraw_balance(const uint64_t& acct,const uint64_t& asset_id,const uint64_t&	quantity)
{
   balances_index balances(_self, acct);
   auto itr = balances.find(asset_id);
   if (itr != balances.end())
   {
      htlcbalance  bal = *itr;
      if(bal.amount < quantity)
         return false;
      balances.modify(*itr, _self, [&](auto& row)
       {
          row.amount -= quantity;
       });
      return true;
   }
   return false;
}

GRAPHENE_ABI(htlc,(createtk)(issuetk)(retiretk)(transfertk)(build)(withdrawhtlc)(refundhtlc))
