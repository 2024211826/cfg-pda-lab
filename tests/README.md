# 测试用例说明

本目录保存可直接喂给程序的测试输入。

运行方式示例：

```powershell
Get-Content .\tests\cfg_sample.txt | .\build\pda_cfg.exe cfg
Get-Content .\tests\pda_sample.txt | .\build\pda_cfg.exe pda
```

批量测试后，输出结果保存到 `tests/output/`。
