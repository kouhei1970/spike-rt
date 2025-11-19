#include <stdlib.h>
#include <stdio.h>
#include <kernel.h>

#include <spike/hub/system.h>

#include <pid_exercise.h>

#include "spike/pup/motor.h"
#include "spike/pup/colorsensor.h"
#include "spike/pup/forcesensor.h"
#include "spike/pup/ultrasonicsensor.h"

#include "spike/hub/battery.h"
#include "spike/hub/button.h"
#include "spike/hub/display.h"
#include "spike/hub/imu.h"
#include "spike/hub/light.h"
#include "spike/hub/speaker.h"

#include <pbio/color.h>

#include "kernel_cfg.h"
#include "syssvc/serial.h"
#include <t_syslog.h>         // システムログ出力

// --- グローバル変数 (SPIKE API) ---
pup_motor_t *motor_left;          // 左モータ(左輪)
pup_motor_t *motor_right;         // 右モータ (右輪)
pup_device_t *color_sensor_left;  // 左カラーセンサー 
pup_device_t *color_sensor_right; // 右カラーセンサー 

// --- ポート定義  ---
#define LEFT_COLOR_SENSOR PBIO_PORT_ID_A
#define RIGHT_COLOR_SENSOR PBIO_PORT_ID_B
#define LEFT_MOTOR        PBIO_PORT_ID_C
#define RIGHT_MOTOR       PBIO_PORT_ID_D

// --- PIDゲインと定数 ---
#define BASE_POWER 100       // 基本走行パワー (0〜100)
#define CONTROL_PERIOD_MS 10 // 制御周期 [ms]
#define CONTROL_PERIOD_US (CONTROL_PERIOD_MS * 1000) // 制御周期 [us]

// ゲインはロボットの特性に合わせて調整してください
const float kp = 1.0;    // 比例ゲイン (P)21
const float ki = 0.1;  // 積分ゲイン (I)10.0
const float kd = 0.1;   // 微分ゲイン (D)0.1
const float delta_T = (float)CONTROL_PERIOD_MS / 1000.0f; // 制御周期 [s]

// --- メイン関数 ---
void Main(intptr_t exinf)
{
    // --- 変数定義 ---
    uint8_t reflection_left, reflection_right;             // 反射光の強さ [%]
    int32_t left_speed, right_speed; // 左右のモータースピード
    
    // ライン情報 (事前に測定・調整が必要)
    //const int white_ref = 80;       // 白の反射光値 (例)
    //const int black_ref = 5;        // 黒の反射光値 (例)
    //const float midpoint = (float)(white_ref + black_ref) / 2.0f; // 目標反射光値 (ラインのエッジ)
    const float midpoint = 0.0f; // 目標反射光値 (ラインの中央）

    // PID制御状態変数
    float lasterror = 0.0f;
    float integral = 0.0f;
    float position = 0.0f;
    float error = 0.0f;
    float mv = 0.0f; // 操作量 (Manipulated Variable) - ステアリング値

    // --- 初期化と設定 ---
    
    // モーター設定 (A:左輪 反時計回り, B:右輪 時計回り)
    motor_left = pup_motor_init(LEFT_MOTOR, PUP_DIRECTION_COUNTERCLOCKWISE);
    motor_right = pup_motor_init(RIGHT_MOTOR, PUP_DIRECTION_CLOCKWISE);
    
    // カラーセンサー設定
    color_sensor_left = pup_color_sensor_get_device(LEFT_COLOR_SENSOR);
    color_sensor_right = pup_color_sensor_get_device(RIGHT_COLOR_SENSOR);
    
    // 初期待機 (3秒)
    dly_tsk(3000000); 

    // --- 走行開始の合図 ---
    hub_speaker_set_volume(100);
    hub_speaker_play_tone(695, 200); // 準備完了の音
    dly_tsk(200000);

    // --- メイン制御ループ ---
    // 無限ループなので場合によって要変更
    while(1) { 
        
        // 1. センサー値の取得
        reflection_left = pup_color_sensor_reflection(color_sensor_left);
        reflection_right = pup_color_sensor_reflection(color_sensor_right);
        position = (float)(reflection_right - reflection_left);
        
        // 2. 偏差(Error)の計算 (目標値 - 現在値)
        error = midpoint - position; 
        //syslog(LOG_NOTICE, "Reflection L:%d R:%d Error:%d", reflection_left, reflection_right, (int8_t)error);

        
        // 3. 積分項(Integral)の計算
        // I = I + ( (今回の誤差 + 前回の誤差) / 2 ) * delta_T
        integral += ((error + lasterror) / 2.0f * delta_T);
        
        // 4. PID制御量の計算 (ステアリング出力 mv)
        float derivative = (error - lasterror) / delta_T; // 微分項
        mv = kp * error + ki * integral + kd * derivative;

        // 5. ステアリング出力の制限
        if (mv > 100.0f) mv = 100.0f;
        if (mv < -100.0f) mv = -100.0f; 

        // 6. モーター速度の決定
        // mvが正: 右旋回 -> 左輪を速く
        left_speed  = (int32_t)((float)BASE_POWER * ((float)BASE_POWER + mv) / (float)BASE_POWER);;
        right_speed = (int32_t)((float)BASE_POWER * ((float)BASE_POWER - mv) / (float)BASE_POWER);

        #if 0
        if (mv >= 0.0f) {
            left_speed  = BASE_POWER;
            right_speed = (int32_t)((float)BASE_POWER * ((float)BASE_POWER - mv) / (float)BASE_POWER);
        } 
        // mvが負: 明るい方へ戻る -> 右輪を速く
        else {
            left_speed  = (int32_t)((float)BASE_POWER * ((float)BASE_POWER + mv) / (float)BASE_POWER);
            right_speed = BASE_POWER;
        }
        #endif

        
        // 7. モーター駆動
        pup_motor_set_speed(motor_left, left_speed);
        pup_motor_set_speed(motor_right, right_speed);

        // 8. 状態変数の更新
        lasterror = error;

        // 9. 制御周期の待機
        dly_tsk(CONTROL_PERIOD_US); // 10ms待機
    }

    // --- 終了処理 ---
    
    // モーターを停止
    pup_motor_stop(motor_left);
    pup_motor_stop(motor_right);

    // 終了音
    hub_speaker_play_tone(784, 200); 
    dly_tsk(200000);
    
    // プログラムを終了します
    exit(0);
}
