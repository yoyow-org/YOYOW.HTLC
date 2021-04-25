#pragma once

#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <graphenelib/contract_asset.hpp>
#include <graphenelib/crypto.h>

#include <algorithm> 

using namespace graphene;
using namespace graphenelib;

#define htlc_account 28182

uint64_t checksum256_to_uint(const checksum256& data)
{
	auto id = data.hash;
	return uint64_t(id[0])<<24 | uint64_t(id[1])<<16 | uint64_t(id[2])<<8 | uint64_t(id[3]);
}

class htlc: public contract
{
   public:   	
   	htlc(uint64_t id)
        : contract(id)
          {};

#include "token.hpp"            	
			

public:
      //@abi action
      void build(const uint64_t& sender, const uint64_t& receiver, const uint64_t& asset_id, const uint64_t& quantity,const checksum256& hashlock, const uint64_t& timelock);

      //@abi action
      void withdrawhtlc(uint64_t id, std::string preimage);

      //@abi action
      void refundhtlc(uint64_t id);

private:
       //@abi table htlcbalance
      struct htlcbalance
      {
         uint64_t    asset_id; 
         int64_t     amount;

         uint64_t primary_key() const { return asset_id; }

         GRAPHENE_SERIALIZE(htlcbalance, (asset_id)(amount));
      };
      typedef multi_index<N(htlcbalance), htlcbalance> balances_index;
      
      
      void transfer_happened(const uint64_t& from, const uint64_t& to,const uint64_t& asset_id,const uint64_t&	quantity, const std::string& memo );

      /****
       * Remove some tokens from the account balance of htlcbalance table
       * @returns true on success, false otherwise
       */
      bool withdraw_balance(const uint64_t& acct,const uint64_t& asset_id,const uint64_t&	quantity);

      //@abi table htlccon
      struct htlccon
      {
         uint64_t key; // unique key
         checksum256 id;
         uint64_t sender; // who created the HTLC
         uint64_t receiver; // the destination for the tokens
         uint64_t	asset_id;
         uint64_t	quantity;
         checksum256 hashlock; // the hash of the preimage
         uint64_t timelock; // when the contract expires and sender can ask for refund
         bool funded;
         bool withdrawn; // true if receiver provides the preimage
         bool refunded; // true if sender is refunded
         std::string preimage; /// the preimage provided by the receiver to claim

         uint64_t primary_key() const { return key; }
         uint64_t by_id() const { return checksum256_to_uint(id); }

         htlccon() {}

         htlccon(const uint64_t& sender, const uint64_t& receiver, uint64_t asset_id,uint64_t	quantity,
               checksum256 hashlock, uint64_t timelock)
         {
            this->sender = sender;
            this->receiver = receiver;
            this->asset_id = asset_id;
            this->quantity = quantity;
            this->hashlock = hashlock;
            this->timelock = timelock;
            this->preimage = "";
            this->funded = false;
            this->withdrawn = false;
            this->refunded = false;
            sha256( reinterpret_cast<char *>(this), sizeof(htlccon),&(this->id)  );
         }

         GRAPHENE_SERIALIZE(htlccon, 
               (key)
               (id)
               (sender)
               (receiver)
               (asset_id)
               (quantity)
               (hashlock)
               (timelock)
               (funded)
               (withdrawn)
               (refunded)
               (preimage)
               );
      };

      /*****
       * Indexing
       */
      typedef multi_index<N(htlccon), htlccon,
            indexed_by<N(id), 
            const_mem_fun<htlccon, uint64_t, 
            &htlccon::by_id>>> htlc_index;

     

      /***
       * check if HTLC contract exists
       */
      bool contract_exists(const checksum256& id); 

};
