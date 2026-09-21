# Progress
2026-09-16 只读检查完成；用户原有修改留在原目录。
读取 CML 时初次 rg 使用 Windows 通配路径失败，改用目录搜索。
资源按 GBK 严格解码失败，后续按行检查混合编码。
- 独立副本仅复制所需 CML 头文件/库、Ixxat 头文件、Eigen 和资源。
- 首次 MSBuild 被沙箱 SDK 访问权限阻断，获准后编译。
- 首次 C++ 编译发现 canOpen.Close 返回 void，已按本地 SDK 修正。
- 修复资源混合编码、原对话框文字及资源 FONT 语法。
- 首轮测试发现反向叶片索引存在有符号/无符号乘法溢出，转为 double 后 27 组测试全部通过。
- Debug x64 初版完整链接成功；Release 构建中。测试未访问任何硬件。
- Computer Use 技能可读，但当前工具列表没有 node_repl，未进行 Windows GUI 自动化或实机测试。
- Release 首次构建 C1047：现有 CML 预编译库与新 /GL 对象版本不兼容。副本关闭 WholeProgramOptimization 后重建，未替换厂商库。
- 最终 Debug x64、Release x64 均编译链接成功；建议使用 Release 交付程序。
- 27 组离线控制测试全部通过，结果保存在 tests/test-results.txt。
- 包检查通过：17 个工程源/资源项齐全，28 个 DDX 控件均存在，Release EXE 为 AMD64，配置保持 Confirmed=0。
- 一次临时单行包检查的正则转义出错，改成 tests/validate_package.py 后通过。
- 原项目已有的 4 个修改文件保持原样；本次新增内容全部位于当前独立子文件夹。
- 未运行真实窗口、未连接 CAN、未启动电机；真实高速同步仍需硬件接口与标定数据。
