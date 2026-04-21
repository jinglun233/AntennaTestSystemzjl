# AntennaTestSystemzjl - Git 使用指南
# =====================================

## 📌 核心概念（3条就够用）

### 1. 工作区 → 暂存区 → 仓库
```
你改了代码 (工作区)  →  git add (暂存区)  →  git commit (本地仓库)  →  git push (GitHub)
```
- **工作区**：你正在编辑的文件（红色 = 改了但没记录）
- **暂存区**：准备提交的文件（绿色 = 准备好要记录）
- **仓库**：已保存的版本快照（可以随时回退到任何一个）

### 2. 常用命令速查

| 操作 | 命令 | 说明 |
|------|------|------|
| **看状态** | `git status` | 看哪些文件被改了 |
| **看具体改了啥** | `git diff` | 看每一行增删 |
| **记录改动** | `git add <文件> && git commit -m "说明"` | 提交一个版本点 |
| **记录所有改动** | `git add . && git commit -m "说明"` | 一把梭 |
| **查看历史** | `git log --oneline` | 所有版本列表 |
| **回退到某版** | `git checkout <commit号>` | 回退（不删代码） |
| **放弃本次改动** | `git checkout -- <文件>` | 丢弃未提交的修改 |
| **上传到 GitHub** | `git push` | 推送到远程 |

### 3. Commit 规范（写清楚改了什么）

```
feat: 新功能
fix: 修复 bug
refactor: 重构（不改功能，优化代码）
docs: 文档变更
style: 格式调整（不影响逻辑）
```

示例：
- `feat: 实现周期性指令1秒发送`
- `fix: 修复环形缓冲区越界问题`
- `refactor: 协议帧格式从变长改为固定1029字节`

## 🔁 典型操作场景

### 场景 A：改完一个功能，想记录一下
```bash
git add .
git commit -m "feat: 修改了xxx"
git push          # 可选：同步到 GitHub
```

### 场景 B：改完了发现改坏了，想撤销
```bash
# 还没 git add → 直接丢弃
git checkout -- 文件名.cpp

# 已经 git add 但还没 commit → 取消暂存
git reset HEAD 文件名.cpp

# 已经 commit 了 → 回退到上一版（保留文件）
git reset HEAD~1
# 或回退到指定版本（彻底回到那个状态）
git reset --hard <commit号>
```

### 场景 C：想看看某个版本的代码长什么样
```bash
git log --oneline           # 先找到版本号
git show <commit号>        # 看那个版本改了什么
git checkout <commit号>    # 切换到那个版本浏览（只读）
git checkout main          # 回到最新版本
```

## ⚠️ 注意事项

1. **不要在 main 分支上直接 reset --hard** — 会丢失代码且无法 push
   - 安全做法：先建个分支 `git checkout -b backup` 再操作
2. **每次改完及时 commit** — 不要攒一堆改动才提交
3. **commit message 写中文也行** — 写清改了什么最重要
4. **.gitignore 已配置好** — build产物、IDE配置不会被记录
