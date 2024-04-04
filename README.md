# HttpServer
## 使用方法
1. 使用`git clone`到你的本地服务器
`git clonehttps://github.com/dtuling/HttpServer.git`
2. 进入到项目 提交的时候没注意目录层级搞多了，要cd三次
`cd HttpServer`
`cd httpserver`
`cd HttpServer-Backup`
3. 一键编译发布
`chmod +x ./build.sh`
`./build.sh`
4. 进入到发布目录下
   `cd output`
5. 启动服务器
`./httpserver`


服务器默认端口号是8081，更改的话进入到mian.cpp文件中更改端口号即可。
确定能够访问到你的服务器，将端口号打开。
