# bin=httpserveer
# cc=g++
# cgi=test_cgi
# LD_FLAGS=-std=c++11 -lpthread #-DDEBUG=1
# src=main.cpp
# curr=$(sgeel pwd) #获取当前工作路径

# .PHONY:ALL
# ALL:$(bin) $(cgi)

# $(bin):$(src)
# 	$(cc) -o $@ $^ $(LD_FLAGS) -g 
# $(cgi):cgi/test_cgi.cc
# 	$(cc) -o $@ $^ 

# .PHONY:clear
# clear:
# 	rm -f $(bin)
# 	rm -rf output

# .PHONY:output #发布
# output:
# 	mkdir -p output
# 	cp $(bin) output
# 	cp -rf wwwroot output
# 	cp $(cgi) output/wwwroot

.PHONY:ALL
ALL:test_cgi httpserver

httpserver:main.cpp
	g++ -o $@ $^ -std=c++11 -lpthread
test_cgi:cgi/test_cgi.cc
	g++ -o $@ $^ -std=c++11

.PHONY:clear
clear:
	rm -rf httpserver
	rm -rf test_cgi
	rm -rf cgi/test_cgi
	rm -rf output

.PHONY:output #打包发布
output:	
	mkdir -p output
	cp httpserver output
	cp -rf wwwroot output
	cp test_cgi output/wwwroot