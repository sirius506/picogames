// Peg Solitaire for Raspberry Pi Pico by KenKen

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "LCDdriver.h"
#include "graphlib.h"
#include "picogames.h"

#define BOARDXSIZE 7
#define BOARDYSIZE 7
#define BALLXSIZE 32
#define BALLYSIZE 32

// グローバル変数定義
static uint32_t keystatus,keystatus2,oldkey; //最新のボタン状態と前回のボタン状態
static unsigned char board[BOARDYSIZE][BOARDXSIZE]; //盤の状態
unsigned char undob[BOARDXSIZE*BOARDYSIZE][BOARDYSIZE][BOARDXSIZE]; //盤の状態
int balls; //ボール残数
int step; //現在の手数
int cursorx1,cursory1,cursorx2,cursory2; //カーソル位置

// 盤初期データ
// 0：穴なし　1：穴（ボールあり）　2：穴（ボールなし）
const unsigned char BOARD[]={
	0,0,1,1,1,0,0,
	0,0,1,1,1,0,0,
	1,1,1,1,1,1,1,
	1,1,1,2,1,1,1,
	1,1,1,1,1,1,1,
	0,0,1,1,1,0,0,
	0,0,1,1,1,0,0
};

// カラーパレットデータ（8～15）
const unsigned char PALDAT[]={
	185,122,87,
	34,177,76,
	136,0,21,
	255,255,255,
	128,0,0,
	0,0,0,
	0,0,0,
	0,0,0
};

// 画像ビットマップ
// BMP1：ボールあり画像　BMP2：穴（ボールなし）画像
const unsigned char BMP1[]={
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,8,8,8,8,8,8,8,
	8,8,8,8,8,8,9,9,9,9,9,9,9,9,11,9,9,9,9,9,9,9,9,9,9,9,8,8,8,8,8,8,
	8,8,8,8,8,8,9,9,9,9,9,9,11,11,11,11,9,9,9,9,9,9,9,9,9,9,8,8,8,8,8,8,
	8,8,8,8,8,9,9,9,9,9,9,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,8,8,8,8,8,
	8,8,8,8,9,9,9,9,9,11,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,8,8,8,8,
	8,8,8,8,9,9,9,9,9,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,8,8,8,8,
	8,8,8,8,9,9,9,9,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,8,8,8,
	8,8,8,9,9,9,9,9,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,8,8,
	8,8,8,9,9,9,9,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,8,8,
	8,8,8,9,9,9,9,11,11,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,8,8,
	8,8,8,9,9,9,9,9,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,8,8,
	8,8,8,9,9,9,9,9,11,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,8,8,
	8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,8,8,
	8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,8,8,
	8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,12,8,8,
	8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,12,8,8,
	8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,12,8,8,8,
	8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,12,12,8,8,8,
	8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,12,8,8,8,8,
	8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,12,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,9,9,12,12,12,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,9,9,12,12,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,12,12,12,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,12,12,12,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};
const unsigned char BMP2[]={
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,10,10,10,10,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,10,10,10,10,10,10,10,10,10,10,10,10,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,10,10,10,10,10,8,8,8,8,8,8,8,8,10,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,10,10,10,10,10,8,8,8,8,8,8,8,8,8,8,8,10,10,8,8,8,8,8,8,8,
	8,8,8,8,8,8,10,10,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,8,8,8,
	8,8,8,8,8,8,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,8,8,8,
	8,8,8,8,8,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,8,8,
	8,8,8,8,10,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,8,
	8,8,8,8,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,8,
	8,8,8,8,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,
	8,8,8,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,
	8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,
	8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,
	8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,11,8,8,8,10,8,8,8,
	8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,11,8,8,8,10,8,8,8,
	8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,11,11,8,8,8,10,8,8,8,
	8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,11,8,8,8,8,10,8,8,8,
	8,8,8,8,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,11,11,8,8,8,10,8,8,8,8,
	8,8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,11,11,8,8,8,8,10,8,8,8,8,
	8,8,8,8,8,10,8,8,8,8,8,8,8,8,8,8,8,8,11,11,11,11,8,8,8,8,10,8,8,8,8,8,
	8,8,8,8,8,8,10,8,8,8,8,8,8,8,8,8,8,11,11,8,8,8,8,8,8,10,8,8,8,8,8,8,
	8,8,8,8,8,8,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,8,8,8,
	8,8,8,8,8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,10,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,10,8,8,8,8,8,8,8,8,8,8,8,8,10,10,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,10,10,8,8,8,8,8,8,8,8,10,10,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,10,10,10,10,10,10,10,10,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};
static const unsigned int SOUND1[]={
	0x64000,1
};
static const unsigned int SOUND2[]={
	0x60800,0x60C00,0x61000,0x61800,0x62000,0x61800,0x61000,0x60C00,0x60800,0x60400,1
};
static const unsigned int SOUND3[]={
	0x306E0,0x30700,1
};
static const unsigned int SOUND4[]={
	0x30B80,0x30C00,1
};
static const unsigned int SOUND5[]={
	0x30400,1
};
static const unsigned int SOUND6[]={
	0x30400,0x20000,0x30400,1
};

static void keycheck(void){
//ボタン状態読み取り
//keystatus :現在押されているボタンに対応するビットを1にする
//keystatus2:前回押されていなくて、今回押されたボタンに対応するビットを1にする
#if 0
	oldkey=keystatus;
	keystatus=~gpio_get_all() & KEYSMASK;
	keystatus2=keystatus & ~oldkey; //ボタンから手を離したかチェック
#endif
   oldkey = keystatus;
   keystatus = get_pad_vmask();
   keystatus2=keystatus & ~oldkey; //ボタンから手を離したかチェック
}

static void sound(const unsigned int *s){
//効果音出力
//以下の32bit列を与える
//上位16bit：音程、2048が440Hz、大きいほど低音
//下位16bit：音の長さ、60分の1秒単位
//最後に上位16bitが0で繰り返し回数を指定
	unsigned int pr,t;
	int n;
	const unsigned int *p;
	p=s;
	while(*p>=0x10000) p++;
	wait60thsec(1);
	for(n=*p;n>0;n--){
		p=s;
		while(*p>=0x10000){
			pr=*p & 0xffff;
			t=*p>>16;
			pr*=36352; //2048で440Hzとなるように補正
			sound_on(pr>>16); //サウンド周波数設定
			wait60thsec(t); //60分のt秒ウェイト
			p++;
		}
	}
	sound_off(); //サウンド停止
}
void score(void){
// ボール残り数表示
	printnum2(184,30,7,8,balls,2);
}
void getundob(void){
//アンドゥバッファからコピー
	int i,j;
	for(i=0;i<BOARDYSIZE;i++){
		for(j=0;j<BOARDXSIZE;j++){
			board[i][j]=undob[step][i][j];
		}
	}
}
void setundob(void){
//アンドゥバッファにコピー
	int i,j;
	for(i=0;i<BOARDYSIZE;i++){
		for(j=0;j<BOARDXSIZE;j++){
			undob[step][i][j]=board[i][j];
		}
	}
}
static void gameinit2(void){
	step=0;
	cursorx1=3;
	cursory1=3;
}
static void gameinit(void){
//起動時1回だけ呼ばれる初期化
	int i,j;
	unsigned char r,g,b;
	const unsigned char *p;
	//カラーパレット初期化
	p=PALDAT;
	for(i=8;i<16;i++){
		r=*p++;
		g=*p++;
		b=*p++;
		set_palette(i,b,r,g);
	}
	clearscreen();
	boxfill(0,0,BALLXSIZE*BOARDXSIZE-1,BALLYSIZE*BOARDYSIZE-1,8);
	printstr(172,20,7,-1,"BALLS");
	printstr(160,162,7,-1,"FIRE:");
	printstr(176,172,7,-1,"SELECT");
	printstr(160,182,7,-1,"START:");
	printstr(176,192,7,-1,"BACK");
	printstr(160,202,7,-1,"START 2s");
	printstr(176,212,7,-1,"RESET");
	p=BOARD;
	for(i=0;i<BOARDYSIZE;i++){
		for(j=0;j<BOARDXSIZE;j++){
			board[i][j]=*p++;
		}
	}
	setundob(); //アンドゥバッファにコピー
	gameinit2();
	//ボタン連続押し防止の初期設定
	keystatus=KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT | KEYSTART | KEYFIRE;
}
void putboard(void){
//盤全体を再描画
	int i,j;
	balls=0;
	for(i=0;i<BOARDYSIZE;i++){
		for(j=0;j<BOARDXSIZE;j++){
			if(board[i][j]==1){
				putbmpmn(j*BALLXSIZE,i*BALLYSIZE,BALLXSIZE,BALLYSIZE,BMP1);
				balls++;
			}
			else if(board[i][j]==2){
				putbmpmn(j*BALLXSIZE,i*BALLYSIZE,BALLXSIZE,BALLYSIZE,BMP2);
			}
		}
	}
	score();
}
void putcursor(int x,int y,unsigned char c){
// カーソル枠を表示
	boxfill(x*BALLXSIZE,y*BALLYSIZE,x*BALLXSIZE+1,y*BALLYSIZE+BALLYSIZE-1,c);
	boxfill(x*BALLXSIZE,y*BALLYSIZE,x*BALLXSIZE+BALLXSIZE-1,y*BALLYSIZE+1,c);
	boxfill(x*BALLXSIZE+BALLXSIZE-2,y*BALLYSIZE,x*BALLXSIZE+BALLXSIZE-1,y*BALLYSIZE+BALLYSIZE-1,c);
	boxfill(x*BALLXSIZE,y*BALLYSIZE+BALLYSIZE-2,x*BALLXSIZE+BALLXSIZE-1,y*BALLYSIZE+BALLYSIZE-1,c);
}
void undocheck(void){
// STARTボタンをチェックし、アンドゥを行う
// 長押しした場合は、初期状態に戻す
	int t=0;
	while(t<120){
		wait60thsec(1);
		keycheck();
		if(keystatus!=KEYSTART) break;
		t++;
	}
	if(t==120){
		//初期状態に戻す
		gameinit2();
		sound(SOUND6);
	}
	else{
		//アンドゥ（1つ前に戻す）
		if(step>0){
			step--;
			sound(SOUND5);
		}
	}
}
int move1(void){
// 移動するボールを選択
// 戻り値　1：STARTボタン押下　0：その他
	while(1){
		putcursor(cursorx1,cursory1,2); //赤カーソル表示
		do{
			wait60thsec(1);
			keycheck();
		} while(keystatus2==0);
		putcursor(cursorx1,cursory1,8); //カーソル消去
		if(keystatus2==KEYSTART){
			// アンドゥまたは初期状態に戻す
			undocheck();
			return 1;
		}
#if 0
		if(keystatus2==KEYFIRE && board[cursory1][cursorx1]==1) break; //ボールのあるところでFIREボタン
#else
		if(keystatus2&KEYFIRE && board[cursory1][cursorx1]==1) break; //ボールのあるところでFIREボタン
#endif
		//上下左右ボタンでカーソル移動
		if(keystatus2==KEYDOWN && cursory1<BOARDYSIZE-1 && board[cursory1+1][cursorx1]) cursory1++;
		if(keystatus2==KEYUP && cursory1>0 && board[cursory1-1][cursorx1]) cursory1--;
		if(keystatus2==KEYLEFT && cursorx1>0 && board[cursory1][cursorx1-1]) cursorx1--;
		if(keystatus2==KEYRIGHT && cursorx1<BOARDXSIZE-1 && board[cursory1][cursorx1+1]) cursorx1++;
	}
	sound(SOUND3);
	return 0;
}
int move2(void){
// ボールの移動先を選択
// 戻り値　1：STARTボタン押下　0：その他
	cursorx2=cursorx1;
	cursory2=cursory1;
	while(1){
		putcursor(cursorx1,cursory1,2); //赤カーソル表示
		putcursor(cursorx2,cursory2,4); //緑カーソル表示
		do{
			wait60thsec(1);
			keycheck();
		} while(keystatus2==0);
		putcursor(cursorx1,cursory1,8); //カーソル消去
		putcursor(cursorx2,cursory2,8); //カーソル消去
		if(keystatus2==KEYSTART){
			// アンドゥまたは初期状態に戻す
			undocheck();
			return 1;
		}
#if 0
		if(keystatus2==KEYFIRE) break; //FIREボタン
#else
		if(keystatus2&KEYFIRE) break; //FIREボタン
#endif

		//上下左右ボタンでカーソル移動
		if(keystatus2==KEYDOWN && cursory2<BOARDYSIZE-1 && board[cursory2+1][cursorx2]) cursory2++;
		if(keystatus2==KEYUP && cursory2>0 && board[cursory2-1][cursorx2]) cursory2--;
		if(keystatus2==KEYLEFT && cursorx2>0 && board[cursory2][cursorx2-1]) cursorx2--;
		if(keystatus2==KEYRIGHT && cursorx2<BOARDXSIZE-1 && board[cursory2][cursorx2+1]) cursorx2++;
	}
	return 0;
}
int movecheck(void){
// ボールが移動できるかチェック
// 戻り値　0：移動できる　1：移動できない
	if(board[cursory2][cursorx2]!=2) return 1; //移動先が開いていない
	if(cursorx1==cursorx2){
		if(cursory1-cursory2!=2 && cursory1-cursory2!=-2) return 1; //距離が2でない
		if(board[(cursory1+cursory2)/2][cursorx1]==1) return 0; //間にボールがある
	}
	else if(cursory1==cursory2){
		if(cursorx1-cursorx2!=2 && cursorx1-cursorx2!=-2) return 1; //距離が2でない
		if(board[cursory1][(cursorx1+cursorx2)/2]==1) return 0; //間にボールがある
	}
	return 1; //筋が違う
}
void move3(void){
// ボールを移動させ、飛び越えたボールを取り除く
	putbmpmn(cursorx2*BALLXSIZE,cursory2*BALLYSIZE,BALLXSIZE,BALLYSIZE,BMP1);
	putbmpmn(cursorx1*BALLXSIZE,cursory1*BALLYSIZE,BALLXSIZE,BALLYSIZE,BMP2);
	putbmpmn((cursorx1+cursorx2)/2*BALLXSIZE,(cursory1+cursory2)/2*BALLYSIZE,BALLXSIZE,BALLYSIZE,BMP2);
	board[cursory2][cursorx2]=1;
	board[cursory1][cursorx1]=2;
	board[(cursory1+cursory2)/2][(cursorx1+cursorx2)/2]=2;
	balls--;
	cursorx1=cursorx2;
	cursory1=cursory2;
	step++;
}
static int goalcheck(void){
//パズル完成チェック
//完成の場合、画面表示してSTARTボタン待ちする
//戻り値　未完成:0、完成:1
	if(balls>1) return 0;
	printstr(48,108,6,-1,"CONGRATULATIONS!");
	printstr(72,130,7,-1,"PUSH START");
	printstr(60,140,7,-1,"TO PLAY AGAIN");
	sound(SOUND2);
	while(keystatus2!=KEYSTART){
		wait60thsec(1);
		keycheck();
	}
	gameinit2();//ゲーム初期状態に戻す
	return 1;
}

void peg_main(void){

	init_graphic(); //液晶利用開始
	LCD_WriteComm(0x37); //画面中央にするためスクロール設定
	LCD_WriteData2(272);

	//ここからゲーム処理
	gameinit();//最初の1回だけの初期化
	while(1){
		getundob(); //アンドゥバッファから戻す
		putboard(); //盤全体の描画

		//メインループ
		while(1){
			if(move1()) break; //移動するボール選択、STARTボタンでアンドゥまたは初期状態に戻る
			if(move2()) break; //移動先選択、STARTボタンで初期状態に戻る
			if(movecheck()){ //移動できるかチェック、不可なら選択からやり直し
				sound(SOUND1);
				continue;
			}
			move3(); //ボールを移動させる
			setundob(); //アンドゥバッファにコピー
			score(); //ボール残り数表示
			if(goalcheck()) break; //完成チェック、完成の場合最初に戻る
			sound(SOUND4);
		}
	}
}
