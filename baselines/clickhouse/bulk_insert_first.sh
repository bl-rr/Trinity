
for i in `seq 0 19`
do
    cat /mntData/tpch_split/x$i | clickhouse-client --query="INSERT INTO tpch FORMAT CSV" &
done

# python3 try_out_query.py 0

exit 0


# client
clickhouse-client --database=default --query="CREATE TABLE IF NOT EXISTS tpch (ID UInt32, QUANTITY UInt8, EXTENDEDPRICE UInt32, DISCOUNT UInt8, TAX UInt8, SHIPDATE UInt32, COMMITDATE UInt32, RECEIPTDATE UInt32, TOTALPRICE UInt32, ORDERDATE UInt32) ENGINE = Distributed(test_trinity, default, tpch, rand())";

# server
clickhouse-client --database=default --query="CREATE TABLE IF NOT EXISTS tpch (ID UInt32, QUANTITY UInt8, EXTENDEDPRICE UInt32, DISCOUNT UInt8, TAX UInt8, SHIPDATE UInt32, COMMITDATE UInt32, RECEIPTDATE UInt32, TOTALPRICE UInt32, ORDERDATE UInt32) Engine = MergeTree ORDER BY (ID)";
