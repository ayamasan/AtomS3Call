// for M5Stack AtomS3

#include <M5AtomS3.h>
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

// アクセスポイント設定
const char ssid[] = "HAS3-01";   // SSID
const char pass[] = "12348765";  // パスワード

int state = 0; // 状態 0:待機, 1:呼び出し受信, 2:応答送信
// クライアントからの呼び出しで「1」をセット
// ボタン押しで「2」をセットし、クライアントに応答送信、待機状態「0」に戻る

// PWM出力 ledc変数
uint8_t ledcGPIO = 2;       // PWM GPIO
uint8_t ledcCH = 0;         // ledcチャンネル
uint8_t ledcBitWidth = 8;   // 8bit 0-255（13bit default）
uint32_t ledcDuty = (pow(2, ledcBitWidth) / 2 ) - 1;   // デューティ比初期値設定 1/2 (127)

// PWM音程用周波数
//                  ド    レ    ミ    ファ  ソ    ラ    シ    ド
//double onkai[] = {2093, 2349, 2637, 2794, 3136, 3520, 3951, 4186};
double onkai[] = {1101, 1236, 1388, 1470, 1650, 1853, 2080, 2203};


///////////////////////////////////////////////////////////////////////////////
// wifiアクセスポイント
void StartAP() 
{
	WiFi.softAP(ssid, pass);  // アクセスポイント開始
	
	// WiFi情報表示
	AtomS3.Lcd.fillScreen(0);     // 背景色
	AtomS3.Lcd.setTextFont(2);    // フォント
	AtomS3.Lcd.setCursor(0, 0);   // 表示開始位置左上角（X,Y）
	AtomS3.Lcd.print(" SSID= ");  // SSID表示
	AtomS3.Lcd.println(ssid);
	AtomS3.Lcd.print(" PASS= ");  // パスワード表示
	AtomS3.Lcd.println(pass);
	AtomS3.Lcd.print(" IP= ");    // IPアドレス表示
	AtomS3.Lcd.println(WiFi.softAPIP());
}


///////////////////////////////////////////////////////////////////////////////
// ルート 時の応答
void handleRoot() 
{
	server.send(200, "text/html", "200 root ok");  // レスポンス200
	AtomS3.Lcd.setCursor(8, 40);
	AtomS3.Lcd.println("ROOT OK");
}


///////////////////////////////////////////////////////////////////////////////
// 見つからない時の応答
void handleNotFound() 
{
	server.send(404, "text/plain", "404 Not Found"); // エラー
}


///////////////////////////////////////////////////////////////////////////////
// call 時の応答
void CallRes() 
{
	server.send(200, "text/html", "200 CALLING");   // レスポンス
	AtomS3.Lcd.clear();
	AtomS3.Lcd.setTextColor(WHITE, RED);  // 文字色,背景色
	AtomS3.Lcd.fillScreen(RED);           // 背景色
	AtomS3.Lcd.setCursor(8, 40);
	AtomS3.Lcd.println("CALLING");
	song();
	state = 1;
}


///////////////////////////////////////////////////////////////////////////////
// ok 時の応答
void OkRes() 
{
	AtomS3.Lcd.clear();
	if(state == 1){
		server.send(200, "text/html", "200 CALLING");   // レスポンス200を返す
		AtomS3.Lcd.setTextColor(RED, BLACK);  // 文字色,背景色
		AtomS3.Lcd.fillScreen(0);             // 背景色
		AtomS3.Lcd.setCursor(8, 40);
		AtomS3.Lcd.println("CALLING");
	}
	else if(state == 2){
		server.send(200, "text/html", "200 AGREE");  //レスポンス200を返す
		AtomS3.Lcd.setTextColor(WHITE, BLACK);  // 文字色,背景色
		AtomS3.Lcd.fillScreen(0);               // 背景色
		AtomS3.Lcd.setCursor(8, 40);
		AtomS3.Lcd.println("AGREE");
		state = 0;
	}
	else{
		server.send(200, "text/html", "200 OK");  //レスポンス200を返す
		AtomS3.Lcd.setTextColor(WHITE, BLACK);  // 文字色,背景色
		AtomS3.Lcd.fillScreen(0);               // 背景色
		AtomS3.Lcd.setCursor(8, 40);
		AtomS3.Lcd.println("*");
	}
	delay(1000);
	if(state != 1){
		AtomS3.Lcd.setCursor(8, 40);
		AtomS3.Lcd.clear();
	}
	else{
		AtomS3.Lcd.setTextColor(WHITE, RED);  // 文字色,背景色
		AtomS3.Lcd.fillScreen(RED);           // 背景色
		AtomS3.Lcd.setCursor(8, 40);
		AtomS3.Lcd.println("CALLING");
	}
}


///////////////////////////////////////////////////////////////////////////////
// 指定音を指定時間鳴らす 0=ド, 1=レ...
void sound(int num, int msec)
{
	ledcSetup(ledcCH, onkai[num], ledcBitWidth);
	ledcWrite(ledcCH, ledcDuty);
	delay(msec);
	ledcWrite(ledcCH, 0);
}

///////////////////////////////////////////////////////////////////////////////
// チャイム「ドレミ」を3回
void song()
{
	for(int i=0; i<3; i++){
		sound(0, 200);
		sound(1, 200);
		sound(2, 200);
		delay(200);
	}
}


///////////////////////////////////////////////////////////////////////////////
// 初期設定
void setup() 
{
	AtomS3.begin();
	delay(1000);
	
	// 液晶初期設定
	AtomS3.Lcd.fillScreen(0);       // 表示クリア
	AtomS3.Lcd.setRotation(0);      // 画面向き設定
	AtomS3.Lcd.setTextFont(4);      // フォント
	AtomS3.Lcd.setTextColor(WHITE, BLACK);  // 文字色,背景色
	
	// 本体をアクセスポイントに設定
	StartAP();
	
	// サーバー設定
	server.on("/", handleRoot);         // ルート時の応答関数
	server.on("/call/", CallRes);       // call 時の応答関数
	server.on("/ok/", OkRes);           // ok 時の応答関数
	server.onNotFound(handleNotFound);  // 見つからない時の応答関数
	server.begin();
	
	// for PWM
	ledcSetup(ledcCH, onkai[0], ledcBitWidth);
	ledcAttachPin(ledcGPIO, ledcCH);
	
	AtomS3.Lcd.setTextFont(4);      // フォント
}


///////////////////////////////////////////////////////////////////////////////
// メインループ
void loop() 
{
	AtomS3.update();                     // ボタン状態初期化
	if(AtomS3.BtnA.wasPressed()){        // ボタンが押されていれば
		if(state == 1){                  // 呼び出し中
			AtomS3.Lcd.setTextColor(WHITE, BLACK);  // 文字色,背景色
			AtomS3.Lcd.fillScreen(0);               // 背景色
			AtomS3.Lcd.setCursor(8, 40);
			AtomS3.Lcd.clear();
			state = 2;  // 応答送信
		}
	}
	else{
		server.handleClient();  //クライアントからのアクセス確認
	}
	delay(100); // 遅延時間
}
