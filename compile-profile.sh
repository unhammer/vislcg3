#  -fno-inline-functions-called-once -fno-inline-small-functions -fno-inline-functions
mkdir -p gcov
rm -fv gcov/*.gcov
rm -fv gcov/*.gcda
rm -fv gcov/*.gcno
rm -fv gcov/vislcg3
cd gcov
g++ -DHAVE_BOOST -I~/tmp/boost_1_48_0 -pipe -Wall -Wextra -Wno-deprecated -O3 -g3 -fno-rtti -fprofile-arcs -ftest-coverage -ffor-scope -L/usr/local/lib -ltcmalloc -licuio -licuuc ../src/all_vislcg3.cpp -o vislcg3
cd ..
mkdir -p gprof
rm -fv gprof/vislcg3
rm -fv gprof/gmon.out
cd gprof
g++ -DHAVE_BOOST -I~/tmp/boost_1_48_0 -pipe -pg -Wall -Wextra -Wno-deprecated -O3 -g3 -fno-rtti -ffor-scope -L/usr/local/lib -ltcmalloc -licuio -licuuc ../src/all_vislcg3.cpp -o vislcg3
