# Vanish - 隐身插件

## 简介

Vanish 是一款可以使管理员隐身的插件，可以让玩家在游戏中看不到管理员。
> 开启了permMode模式后，请勿重载此插件，否则/v命令可能无法使用

## 遇到问题怎么办？
- 提 issue
- 加入 QQ 群 985991178

## 功能
- 完全隐藏玩家，包括玩家本身，手持物，装备，药水粒子等一切显示的内容
- 隐藏进服/退服/修改皮肤/睡床/死亡信息玩家名的文本显示
- 隐藏箱子，潜影盒，木桶的开/关音效和动画
- 隐藏地图/list命令显示
- 隐藏移动/掉落到方块上音效
- 隐藏暂停菜单页面中玩家列表中的玩家
- 睡觉逻辑不计算隐身玩家
- 经验球等不会追踪隐身玩家
- 目标选择器无法选中隐身玩家(控制台和自己除外)
- 隐身后生存模式不吸引怪物仇恨
- 隐藏服务器信息(motd)玩家真实数量

> 目前空手攻击未隐藏，有办法者可pr。

## 命令使用
- `/vanish` - 打开隐身菜单GUI
- 以下命令仅开启`permMode`时存在
- `/vanish add <玩家>` - 添加玩家隐身权限
- `/vanish remove <玩家>` - 移除玩家隐身权限

## 配置文件
```jsonc
{
    "version": 2, // 配置文件版本(勿动)
    "command": "vanish", // 命令名
    "alias": "v", // 命令别名
    "bossbarID": 92732814, // BossbarID(可以不管)
    "enabledPAPI": true, // 启用PAPI变量
    "permMode": false, // 权限模式(如果启用,除非控制台给予权限,否则不可使用隐身命令)
    "permPlayers": [], // 有隐身权限的玩家(建议使用指令修改)
    "playerConfigs": { // 玩家配置(建议使用/v指令设置)
        "玩家的uuid": { // 这里做个示例
            "enabled": true, // 是否启用隐身
            "appearPromptJoin": true, // 解除隐身是否提示加入游戏
            "vanishPromptExit": true, // 隐身后是否提示退出游戏
            "vanishBossbar": true, // 隐身后是否显示Boss栏
            "vanishBossbarText": "§6爷隐身了OvO", // Boss栏的文本
            "vanishNoTakeItem": false, // 隐身后是否可以捡起物品
            "vanishNoDropItem": false // 隐身后是否可以丢弃物品
        }
    }
}
```

## 安装方法
- 手动安装
  - 前往[Releases](https://github.com/zimuya4153/Vanish/releases)下载最新版本的`Vanish-windows-x64.zip`
  - 解压`压缩包内的`文件夹到`./plugins/`目录
- Lip 安装
  - 输入命令`lip install -y github.com/zimuya4153/Vanish`
- ~~一条龙安装~~
  - ~~去 Q 群，喊人，帮你安装~~

## 接口导出
> 无导出接口
但你可以通过`Player::isInvisible`获知玩家是否为隐身状态
以js为例，比如我要写一个选择玩家的表单。
```javascript
mc.regPlayerCmd('test', 'Test', player => {
    const form = mc.newSimpleForm(), players = mc.getOnlinePlayers().filter(player => !(ll.listPlugins().includes("vanish") && player.isInvisible));
    form.setTitle("选择玩家");
    players.forEach(player => form.addButton(player.realName));
    player.sendForm(form, (player, id) => {
        player.tell(`你选择了玩家 ${players[id].realName}`);
    });
}, PermType.Any);
````

## 插件历史
- 起初，这只是种子随手写的屎山插件
- 然后子沐因为想学C++，跟种子要来了源码
- 当时啥也不懂，所以压根不知道怎么适配LL3
- 后来懂了一些，就开始尝试适配了LL3，也是我的第一个C++插件，并发布了minebbs
- (虽然也很屎就是了)
- 后来逐渐挨鲲佬骂，学会了C++的一点点皮毛，也写了很多C++插件
- 然后后面觉得隐身还是有非常多的不足，且自己会的更多了，于是就打算重构
- 最终花了一周的时间，完成了重构，有了现在的隐身2.0版本

# 致谢名单
-  感谢[出众年华](https://github.com/luckyoldboy)帮我找bug o(TωT)o 
- 感谢[Ovila](https://github.com/MAUVE1NE),[刀佬](https://github.com/smartcmd),[Killcerr](https://github.com/killcerr),帮助我编写这个插件。
- 感谢项目[EndstoneMC](https://github.com/EndstoneMC/endstone)