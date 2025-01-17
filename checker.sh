# Build
make

# ./rtree queryfile maxcap d output
echo Test case 1
./rtree Testcases/TC_1/queries_2_10_100_100.txt 10 2 output_tc1.txt
diff output_tc1.txt Testcases/TC_1/answer_2_10_100_100.txt

echo Test case 2
./rtree Testcases/TC_2/queries_2_10_100_1000.txt 10 2 output_tc2.txt
diff output_tc2.txt Testcases/TC_2/answer_2_10_100_1000.txt

echo Test case 3
./rtree Testcases/TC_3/queries_3_10_1000_10000.txt 10 3 output_tc3.txt
diff output_tc3.txt Testcases/TC_3/answer_3_10_1000_10000.txt

echo Test case 4
./rtree Testcases/TC_4/queries_5_20_1000_1000.txt 20 5 output_tc4.txt
diff output_tc4.txt Testcases/TC_4/answer_5_20_1000_1000.txt

echo Test case 5
./rtree Testcases/TC_5/queries_2_10_100000_100000.txt 10 2 output_tc5.txt
diff output_tc5.txt Testcases/TC_5/answer_2_10_100000_100000.txt

echo Test case 6
./rtree Testcases/TC_6/queries_2_10_100_100.txt 10 2 output_tc6.txt
diff output_tc6.txt Testcases/TC_6/answer_2_10_100_100_bulk.txt

echo Test case 7
./rtree Testcases/TC_7/queries_2_10_100_100.txt 10 2 output_tc7.txt
diff output_tc7.txt Testcases/TC_7/answer_2_10_100_100_insert.txt


