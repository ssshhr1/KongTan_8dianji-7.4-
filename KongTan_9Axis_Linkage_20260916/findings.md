# Findings
现有 MotorIds=1..8；CML 容量 12，节点号按 id-1 索引。旧 UI 启动即打开并使能、退出先回零。
旧 EnableMotorBus 中途失败不撤销前轴；CloseMotorBus 依赖 m_bOpen，部分初始化失败不清理。
旧八轴运动使用串行 MoveRel/WaitMoveDone，不构成硬件同步；IK 当前只解算 1..6 轴，7/8 手动。
进给轴未知行程和节点，使用 INI 配置门控。保留原八轴限位数值及运动学。
资源脚本包含混合编码，需在副本内修复乱码及编译语法。
C 盘可用约 3.16 GB；副本只包含源码和所需依赖，不复制历史构建/缓存。
Release 库兼容性：关闭工程 /GL 后现有 CML.lib 可以与本机 v143 工具链链接。保留厂商库原样。
Debug 构建链接原 Release CML 库仍有 CRT 冲突警告；交付建议使用 Release。厂商 PDB 缺失和旧 Eigen 索引类型转换警告仍存在。
