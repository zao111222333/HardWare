## 说明

这是一个F1C200S小板的原理图和PCB。

挖坑网：https://whycan.cn/t_3688.html



用KiCAD 5.1.5绘制。



库基于WJQ，其他器件和封装是KiCAD自带库。

WJQ库：https://github.com/wujique/KiCAD_Lib



gerber目录是投板文件。

bom目录是器件丝印和资料，用InteractiveHtmlBom生成。



**2020-02-29**

已经验证：

USB下载固件到SPI FLASH能启动，TTL能进控制台，RGB LCD能显示、触摸屏。

可以从TF卡启动。

未验证：

TV和音频输出、外扩接口。

如有需要，请联系：www.wujique.com