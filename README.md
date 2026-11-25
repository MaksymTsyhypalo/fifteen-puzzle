Exemplary commands for search algorithms:
./solver --bfs LRUD --metrics < 2_move_test.txt
./solver --dfs LRUD --metrics < 2_move_test.txt
./solver --idfs LRUD --metrics < 2_move_test.txt
./solver --bf LRUD --metrics < 2_move_test.txt
./solver --astar 1 --metrics < 2_move_test.txt
./solver --sma 2 --metrics < 2_move_test.txt
./solver --dfs LRUD --max_depth 4 --metrics < 2_move_test.txt
./solver --sma 2 --sma-mem 15 --metrics < 2_move_test.txt

./solver --sma 2 --sma-mem 15 --metrics < 2_move_test.txt > solution.txt

Command for Python UI test:
python3 viewer.py 2_move_test.txt solution.txt
