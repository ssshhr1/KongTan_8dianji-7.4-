# Findings
- 既有九轴端口把进给轴加入 MotorIds 并按 CML CAN 节点初始化，不能直接驱动 DM542。
- 既有手动中段按钮使用多轴经验补偿，新增明确三段分组入口须独立定义。
- 工作区原始 Dlg.cpp 有未提交修改；不覆盖原项目。
- 发现原 CML SetMotorVelocity 的 SetProfileConfig 错误被 SetCountsPerUnit 覆盖；新副本调整顺序并分别检查。
- 三段新入口采用拮抗对 1/2、3/4、5/6、7/8；原经验补偿与旧逆解不用于此版本，避免把旧模型当成三段全模型。
- Mega 型号已获用户确认；PUL/DIR/ENA、COM 号、每毫米脉冲数未确认。固件默认不产生任何引脚输出。
- 默认进给速度上限同时受 0.5 mm/s 与 200 pulses/s 限制，默认段长 0.25 mm；均为拟定调试值待实测。
