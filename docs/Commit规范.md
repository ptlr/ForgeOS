# <h1 align="center">Commit message规范</h1>

## 一、Commit message 的格式

每次提交，Commit message 都包括三个部分：Header， Body 和 Footer。

```
<type>(<scope>): (subject)
// 空行
<body>
// 空行
<footer>
```

其中，Header是必须的，Body 和 Footer可以省略。

为避免影响美观，每行字符不得超过100个字符。

### 1、Header

Header部分只有一行，包含三个字段：`type`(必须)、`scope`(可选)、`subject`（必须）。

#### 1.1 type

| commit类别 | 说明                                          | 备注       |
| ---------- | --------------------------------------------- | ---------- |
| feat       | 新功能（feature）                             |            |
| perf       | 性能，体验优化                                |            |
| fix        | 修补bug                                       |            |
| docs       | 文档（documentation）                         |            |
| style      | 格式修改，不影响代码运行效果                  | 包括路径等 |
| ref        | 重构代码，即不新增功能，也不修改bug的代码变动 |            |
| env        | 项目环境的变动                                |            |

其中，feat、perf和fix会被写入Change log中。

#### 1.2 scope

`scope`用于说明commit的影响范围，比如内核层，用户层。

#### 1.3 subject

`subject`是commit目的的简短描述，不超过75个字符。

以动词开头，使用第一人称现在时，比如：`change XXX`,第一个字母小写，句尾不加标点

### 2、Body

Body部分是对本次commit的详细描述，可写成多行。

应说明代码变动的动机以及与以前行为的对比。

### 3、Footer

Footer部分用于两种状况。

#### 3.1 不兼容变动

与上一版本不兼容时，以`BREAKING CHANGE`开头，后面是对变动的描述，以及变动理由和迁移方法。

