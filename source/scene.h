//==================================================================================================================
//
// 描画[scene.h]
// Author : Seiya Takahashi
//
//==================================================================================================================
#ifndef _SCENE_H_
#define _SCENE_H_

//==================================================================================================================
// インクルードファイル
//==================================================================================================================
#include "main.h"

//==================================================================================================================
//
// シーンクラスの定義
//
//==================================================================================================================
class CScene
{
public:

	//=============================================================================
	// 描画順番
	//=============================================================================
	typedef enum
	{
		PRIORITY_MESH_SPHERE = 0,		// メッシュ球
		PRIORITY_FIELD,					// フィールド
		PRIORITY_PLAYER,				// プレイヤー
		PRIORITY_OBJECT,				// オブジェクト
		PRIORITY_STONE,					// ストーン
		PRIORITY_EFFECT,				// エフェクト
		PRIORITY_COLLISION,				// 衝突判定
		PRIORITY_SHADOW,				// 影
		PRIORITY_GAME_FADE,				// ゲームフェード
		PRIORITY_UI,					// UI
		PRIORITY_MAX					// 最大
	} PRIORITY;							// 描画順番

	CScene(PRIORITY type = PRIORITY_FIELD);					// コンストラクタ
	virtual~CScene();										// デストラクタ
	virtual void Init(void) = 0;							// 初期化処理
	virtual void Uninit(void) = 0;							// 終了処理
	virtual void Update(void) = 0;							// 更新処理
	virtual void Draw(void) = 0;							// 描画処理

	static void UpdateAll(void);							// 全ての更新処理
	static void DrawAll(void);								// 全ての描画処理
	static void ReleaseAll(void);							// 全てに死亡フラグ立てる処理
	void Release(void);

	void Deleate(int type);					// 削除する

	CScene *GetScene(PRIORITY nPriority, int nCntScene);			// シーン取得
	PRIORITY GetPriority(void) { return m_nPriority; }		// プライオリティの取得

	void SetPriority(PRIORITY priority) { m_nPriority = priority; }	// プライオリティの設定

protected:

private:
	static CScene *m_pTop[PRIORITY_MAX];					// 先頭オブジェクトへのポインタ
	static CScene *m_pCur[PRIORITY_MAX];					// 現在(最後尾)へのポインタ

	CScene *m_pPrev;										// 前のオブジェクトへのポインタ
	CScene *m_pNext;										// 次のオブジェクトへのポインタ

	bool		m_bDeth;									// 死亡フラグ
	PRIORITY	m_nPriority;								// プライオリティ
};
#endif