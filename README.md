# YOYOW.HTLC
YOYOW.HTLC是YOYOW基金会推出的HTLC跨链协议，将支持与其他链通过HTLC协议进行跨链交易。


命令行测试步骤说明
0,编译合约
gxx -g htlc.abi htlc.cpp
gxx -o htlc.wast htlc.cpp

1,布置合约（先将步骤0编译的htlc.abi和htlc.wasm拷贝至yoyow_client所在目录的htlc子目录内）
deploy_contract 28182  0 0 ./htlc false true

2,创建2种token

call_contract  28182  28182  null createtk  "{\"issuer\":\"28182\", \"asset_id\":\"10001\",\"maximum_supply\":\"1000000\",\"tkname\":\"asset1\",\"precision\":\"8\"}" false true

call_contract  28182  28182  null createtk  "{\"issuer\":\"28182\", \"asset_id\":\"10002\",\"maximum_supply\":\"1000000\",\"tkname\":\"asset2\",\"precision\":\"8\"}" false true

3,发行2种token

call_contract  28182  28182  null issuetk  "{\"to\":\"28182\", \"asset_id\":\"10001\",\"quantity\":\"100000\",\"memo\":\"testmemo\"}" false true

call_contract  28182  28182  null issuetk  "{\"to\":\"28182\", \"asset_id\":\"10002\",\"quantity\":\"100000\",\"memo\":\"testmemo\"}" false true

4,转账token

call_contract  28182  28182  null transfertk  "{\"from\":\"28182\",\"to\":\"27662\", \"asset_id\":\"10001\",\"quantity\":\"10000\",\"memo\":\"testmemo1\"}" false true

call_contract  28182  28182  null transfertk  "{\"from\":\"28182\",\"to\":\"27447\", \"asset_id\":\"10001\",\"quantity\":\"10000\",\"memo\":\"testmemo1\"}" false true

call_contract  28182  28182  null transfertk  "{\"from\":\"28182\",\"to\":\"27662\", \"asset_id\":\"10002\",\"quantity\":\"10000\",\"memo\":\"testmemo2\"}" false true

call_contract  28182  28182  null transfertk  "{\"from\":\"28182\",\"to\":\"27447\", \"asset_id\":\"10002\",\"quantity\":\"10000\",\"memo\":\"testmemo2\"}" false true


5,HTLC转账

call_contract  27662  28182  null transfertk  "{\"from\":\"27662\",\"to\":\"28182\", \"asset_id\":\"10002\",\"quantity\":\"10000\",\"memo\":\"testmemo1\"}" false true


6,创建htlc

call_contract  27662  28182  null build "{\"sender\":\"27662\",\"receiver\":\"27662\",\"asset_id\":\"10002\",\"quantity\":\"10000\",\"hashlock\":\"5899575803417E3356A133C51EFFF8314C0D3D7A52F37472F90B1DCE5288525B\",\"timelock\":\"1635635929\"}" false true

7,凭密码提取金额

call_contract  27662  28182  null withdrawhtlc  "{\"id\":\"0\",\"preimage\":\"SuperSecret\"}" false true


8,若交易失败，退回资金

call_contract  27662  28182  null refundhtlc  "{\"id\":\"0\"}" false true

*****查询相关操作*****

查询ABI:

get_account init10

get_account 28182


查询合约有哪些表

get_contract_tables init10

get_contract_tables 28182


查询asset_id为10001的资产

get_table_objects  28182  10001 false currencysta 0 -1 100


查询账号uid为27662的各资产余额

get_table_objects  28182  27662 false account 0 -1 100


查询账号27662的HTLC转账

get_table_objects  28182  27662 false htlcbalance 0 -1 100


查看指定htlc详情

get_table_objects  28182  28182 false htlccon 0 -1 100


更新合约

update_contract  init10 ./htlc false true
