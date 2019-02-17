FixCometAssistant
=================

[彗星小助手](http://www.it608.com/Item/ca.html) 是个很方便的工具，窗口网页 Spy 很好用，但是由于已经 5 年没有更新，在 64 位系统上出现了兼容性问题。

这个补丁用于修复彗星小助手不能显示 64 位进程的程序路径的问题。

下载
----

<https://github.com/wzv5/FixCometAssistant/releases/latest>

使用方法
--------

把下载到的 `msimg32.dll` 与 彗星小助手的 exe 放到同一个目录下即可。

注意：由于利用了 dll 劫持注入技术，请将彗星小助手和补丁 dll 放在一个单独的目录中，避免影响到其他程序的运行。

编译
----

使用 [vcpkg](https://github.com/Microsoft/vcpkg) 管理第三方依赖，请自行安装 vcpkg，并安装它的 [VS integrate](https://github.com/Microsoft/vcpkg/blob/master/docs/users/integration.md)。

``` shell
vcpkg install mhook:x86-windows-static
```

原理
----

彗星小助手使用 `CreateToolhelp32Snapshot` 和 `Module32First` 来获取进程路径，但这 2 个 API 对 64 位进程无效。

对于获取进程路径这个需求，微软[推荐](https://docs.microsoft.com/zh-cn/windows/desktop/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot)使用 `QueryFullProcessImageName`。

我大概反汇编看了下，貌似彗星小助手中，有且只有获取进程路径时调用了 `CreateToolhelp32Snapshot`，这就很好办了，通过 hook 以下 API：

- CreateToolhelp32Snapshot
- Module32First
- CloseHandle（用于扫尾清理）

把调用转向 `QueryFullProcessImageName` 就行了。

具体可以看 `FixCometAssistant.cpp`，很简单的。
