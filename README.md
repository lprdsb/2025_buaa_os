总所周知，buaa的机房很老，os上机时碰到这些就太有生活了。

并且机房没有vscode，没有ide，写实验只可以在那个网站上用网页端的命令行写（不仅体验垃圾还有很大的延迟）。

不过线下大伙可以用vscode写降低一点工作量。

但是上机的时候电脑没有各种环境该咋办呢，有没有办法不被电脑的场外因素干扰呢？



有的，兄弟，有的。

像我一样离线安装就好了。

建议在练习的那一周先去机房试一下，尝试在机房没有网络的情况搭一下环境，看看能不能好好用vscode。







# 准备工作（课下提前完成）

从仓库下载

`e4503b30fc78200f846c62cf8091b76ff5547662.tar.gz` 

`OpenSSH-Win64.zip`

`VSCodeUserSetup-x64-1.70.2.exe)`

`config`

`ms-vscode-remote.remote-ssh-0.84.0.vsix`

并把这些玩意传到你的git.os上去，毕竟考试环境会断网。

这样你才可以在机房断网的环境下下载这些环境。

# 获取相关信息

## lab.os

登录https://lab.os.buaa.edu.cn/

点击我的资产

![image-20250727222051007](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727222051007.png)

找到ip那一行

![image-20250727222215416](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727222215416.png)

记录你的IP，比如这里是114.514.1919.810，以后用\<IP\>来表示





然后点击个人信息

![image-20250727222422964](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727222422964.png)

选择重置并下载SSH密钥

![image-20250727222453161](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727222453161.png)

你会得到一个文件叫做`<IP>-jumpserver.pem`

然后把这玩意传到git.os上去。



至此，需要下载的东西结束了



# 安装SSH

在机房的电脑上解压`OpenSSH-Win64.zip`

里面的东西是这样

![image-20250727222915588](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727222915588.png)

假设这个文件路径是`F:\Documents\Downloads\OpenSSH-Win32\OpenSSH-Win32`，以后用\<PATH\>来代指这玩意

## 添加环境变量

![image-20250727223228585](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727223228585.png)

双击这一行

![image-20250727223331143](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727223331143.png)

在末尾加上一个`;`，然后输入\<PATH\>

这时就可以测试cmd中是不是可以调用ssh了

打开cmd

输入`ssh -V`

会得到

![image-20250727223455143](F:\Documents\Workspace\Os\新建文件夹\image-20250727223455143.png)

说明安装好了

## 继续安装

在cmd中输入`ssh-keygen -t rsa`然后一路回车。

不出意外会在`C:\Users\Administrator`中发现`.ssh`文件（记得提前把windows查看隐藏文件和后缀打开）。

这个时候在这个`.ssh`之下创建`config`文件（没有任何后缀）

就像这样

![image-20250727223820750](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727223820750.png)

用记事本打开，编辑

```txt
Host os-lab
    HostName lab.os.buaa.edu.cn
    User <学号>@git@<IP>
    Port 2222
    IdentityFile C:\Users\Administrator\.ssh\<学号>-jumpserver.pem
```

保存，

比如，我这里就是

```txt
Host os-lab
    HostName lab.os.buaa.edu.cn
    User 114514@git@114.514.1919.810
    Port 2222
    IdentityFile C:\Users\Administrator\.ssh\114514-jumpserver.pem
```



然后将之前在lab.os下载的`<IP>-jumpserver.pem`文件复制到`.ssh`文件下。



## 测试

这个时候，应该能使用windows自带的cmd连接服务器了

在cmd输入`ssh os-lab`

![image-20250727224239809](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727224239809.png)

![image-20250727224544708](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727224544708.png)

注意第一次连接会冒出一大段英文，记得输一个`yes`就行了。





# vscode

安装仓库内的vscode

## 安装插件

![image-20250727224809275](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727224809275.png)

这样就可以离线安装remote-ssh插件，注意仓库里的这个和vscode版本是配套的其他版本无法装上



# 服务器（可以课下完成）

## 上传

通过git.os将`e4503b30fc78200f846c62cf8091b76ff5547662.tar.gz`文件上传到服务器的linux机器里（先传到git.os的某个分支，然后在服务器里面git pull）



以下步骤在服务器的linux环境下进行

然后在该文件同目录下新建`setup.sh`文件

写入

```sh
id=e4503b30fc78200f846c62cf8091b76ff5547662
mkdir tmp
tar -xvzf $id.tar.gz -C tmp
rm -r ~/.vscode-server/bin/$id
mv tmp/vscode-server-linux-x64 ~/.vscode-server/bin/$id
rm -r tmp
```

执行`bash setup.sh`

这个时候你可以在通过指令`ls ~/.vscode-server/bin`看到该目录下有对应的文件夹

![image-20250727225500681](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727225500681.png)



基本工作都完成了

# 通过vscode连接



这时，机房的电脑上已经安装了vscode，以及remote ssh插件

然后通过插件连接即可连接到服务器

![image-20250727225658678](F:\Documents\Workspace\Os\新建文件夹\img\image-20250727225658678.png)

然后就可以突破机房的技术封锁了（

