#ifndef TASKS_H
#define TASKS_H

// 各タスクの宣言
void IO_Task();
void Logic_Task();
void UI_Task();

// 初期化関数
void initTasks();
float getInitialTemperature();  // センサー接続確認用

#endif
