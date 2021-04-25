public:
	//@abi action
	void createtk( const uint64_t& issuer,const uint64_t& asset_id,const int64_t& maximum_supply,const name& tkname,const uint8_t& precision)
	{
		graphene_assert( get_trx_sender() == issuer,"can't create token to other user" );
	
	    graphene_assert( asset_id > 10000, "token asset id must bigger than 10000" );
	    graphene_assert( maximum_supply > 0, "max-supply must be positive");
	    
	    stats statstable( _self, asset_id );
	    
	    graphene_assert( statstable.find(asset_id) == statstable.end(), "token with asset id already exists" );
	
	    statstable.emplace( _self, [&]( auto& s ) {
	        s.asset_id = asset_id;
	        s.max_supply = maximum_supply;
	        s.supply = 0;
	        s.tkname = tkname;
	        s.precision = precision;
	        s.issuer  = issuer;
	    });
	}
	
	//@abi action
	void issuetk( const uint64_t& to,const uint64_t& asset_id, const int64_t& quantity, const std::string& memo )
	{
		graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );
		
		stats statstable( _self, asset_id );
	
		auto existing = statstable.find( asset_id);
		graphene_assert( existing != statstable.end(), "token with asset id does not exist, create token before issue" );
		const auto& st = *existing;
		graphene_assert( to == st.issuer, "tokens can only be issued to issuer account" );
	
		graphene_assert( get_trx_sender() == st.issuer ,"invalid authority" );
		graphene_assert( quantity > 0, "must issue positive quantity" );
	
		graphene_assert( quantity <= st.max_supply - st.supply, "quantity exceeds available supply");
	
		statstable.modify( st, 0, [&]( auto& s ) {
		   s.supply += quantity;
		});
	
		add_balance( st.issuer,asset_id,quantity, st.issuer );
	}
	//@abi action
	void retiretk( const uint64_t& asset_id, const int64_t& quantity, const std::string& memo )
	{
		graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );
		
		stats statstable( _self, asset_id );
	
		auto existing = statstable.find( asset_id );
		graphene_assert( existing != statstable.end(), "token with asset id does not exist" );
		const auto& st = *existing;
	
		graphene_assert( get_trx_sender() == st.issuer ,"invalid authority" );
		graphene_assert( quantity > 0, "must retire positive quantity" );
	
		statstable.modify( st, 0, [&]( auto& s ) {
		   s.supply -= quantity;
		});
	
		sub_balance( st.issuer,asset_id,quantity );
	}
	//@abi action
	void transfertk( const uint64_t&    from,
                    const uint64_t&    to,
                    const uint64_t& asset_id,
                    const int64_t& quantity,
                    const std::string&  memo )
	{
		graphene_assert( from != to, "cannot transfer to self" );
		graphene_assert( get_trx_sender() == from,"invalid authority" );
		
		char toname[32];	
		graphene_assert( get_account_name_by_id(toname,32,to) != -1, "to account does not exist");
		
		stats statstable( _self, asset_id );
		
		auto existing = statstable.find( asset_id );
		
		graphene_assert( existing != statstable.end(), "token with asset id does not exist" );
		
		graphene_assert( quantity > 0, "must transfer positive quantity" );
		graphene_assert( memo.size() <= 256, "memo has more than 256 bytes" );
		
		auto payer = from;
		
		sub_balance( from,asset_id,quantity );
		add_balance( to,asset_id,quantity, payer );
		
		if(to == htlc_account)
		{
			transfer_happened(from,to,asset_id,quantity,memo);
		}
	}                    
                    
                        
private:
    const uint64_t get_balance(const uint64_t& account, const uint64_t& asset_type)const
    {
				accounts acnts( _self, account );
				
		    auto itr = acnts.find( asset_type );
		    if( itr == acnts.end() )
		        return 0;
		    else
		        return itr->amount;
		}
    void sub_balance( const uint64_t& owner, const uint64_t& asset_id, const int64_t& value )
    {
    	 accounts acnts( _self, owner );
		   const auto& from = acnts.get( asset_id , "no balance object found" );
		   graphene_assert( from.amount >= value, "overdrawn balance" );
		
		   acnts.modify( from, owner, [&]( auto& a ) {
				 a.amount -= value;
			  });
		}
    void add_balance( const uint64_t& owner, const uint64_t& asset_id, const int64_t& value, const uint64_t& ram_payer )
    {
    	 accounts acnts( _self, owner );
		   auto to = acnts.find( asset_id );
		   if( to == acnts.end() ) {
			  acnts.emplace( ram_payer, [&]( auto& a ){
		        a.asset_id = asset_id;
		        a.amount = value;
			  });
		   } else {
			  acnts.modify( to, 0, [&]( auto& a ) {
		        a.amount += value;
			  });
		   }
		}

public:
    //@abi table currencysta
     struct  currencysta {
        uint64_t    asset_id;
        int64_t     supply;
        int64_t     max_supply;
        uint8_t			precision;
	    	name 	    	tkname;
        uint64_t    issuer;

        uint64_t primary_key()const { return asset_id; }

		GRAPHENE_SERIALIZE(currencysta, (asset_id)(supply)(max_supply)(precision)(tkname)(issuer))
     }; 
    
    typedef multi_index< N(currencysta), currencysta > stats;


//@abi table account
	struct account {
        uint64_t    asset_id; 
        int64_t     amount;
        uint64_t primary_key()const { return asset_id; }

        GRAPHENE_SERIALIZE(account, (asset_id)(amount))
     };
    typedef multi_index<N(account), account> accounts;
    