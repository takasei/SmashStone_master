//==================================================================================================================
//
// 影ポリゴン[shadow.h]
// Author:Seiya Takahashi
//
//==================================================================================================================
#ifndef _SHADOW_H_
#define _SHADOW_H_

//==================================================================================================================
// インクルードファイル
//==================================================================================================================
#include "main.h"
#include "scene.h"

//==================================================================================================================
// マクロ定義
//==================================================================================================================
#define SHADOW_TEX "data/TEXTURE/shadow000.jpg"				// 読み込むテクスチャのソース先

//==================================================================================================================
//
// 影クラスの定義
//
//==================================================================================================================
class CShadow : public CScene
{
public:

	CShadow(PRIORITY type);						// コンストラクタ
	~CShadow();									// デストラクタ
	void Init(void);							// 初期化処理
	void Uninit(void);							// 終了処理
	void Update(void);							// 更新処理
	void Draw(void);							// 描画処理

	static CShadow *Create(void);				// 生成処理
	static HRESULT Load(void);					// テクスチャ情報ロード
	static void Unload(void);					// テクスチャ情報アンロード

	inline D3DXVECTOR3 &GetPos(void) { return m_pos; }		// 位置取得
	void SetPos(D3DXVECTOR3 &pos, D3DXVECTOR3 &move, bool &bJump);// 位置設定
	void ReleaseShadow(void);								// 影の削除

protected:

private:
	static LPDIRECT3DTEXTURE9 m_pTexture;		// テクスチャへのポインタ
	LPDIRECT3DVERTEXBUFFER9 m_pVtxBuff;			// バッファ
	VERTEX_3D *m_pVtx;							// 頂点格納
	D3DXMATRIX m_mtxWorld;						// ワールドマトリックス

	D3DXVECTOR3 m_pos;							// 位置
	D3DXVECTOR3 m_move;							// 移動
	D3DXVECTOR3 m_size;							// 大きさ
	D3DXVECTOR3 m_rot;							// 回転

	bool m_bJump;								// ジャンプフラグ
};
#endif