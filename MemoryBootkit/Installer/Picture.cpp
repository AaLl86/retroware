#include "stdafx.h"
#include "Picture.h"
#include "PeResource.h"

#include <Ole2.h>
#pragma comment (lib, "Ole32.lib")

CPicture::CPicture(void):
	m_gImage(NULL)
{
}

CPicture::CPicture(UINT nResId) 
{
	CreatePicture(nResId);
}

CPicture::~CPicture(void)
{
	DestroyPicture();
}

// Create an Image starting from a Res ID
bool CPicture::CreatePicture(UINT nResId) {
    bool bResult = false;
	Image * pImg = NULL;					// GDI+ Image
    IStream* pStream = NULL;				// COM+ IStream
	DWORD sizeOfRes = 0;					// Size of extracted resource
	LPBYTE resData = NULL;					// Extracted resource HGLOBAL pointer
	HGLOBAL heapGlobal = NULL;
	LPVOID heapMem = NULL;

	resData = CPeResource::Extract(MAKEINTRESOURCE(nResId), L"PNG", &sizeOfRes);
	heapGlobal = GlobalAlloc(GMEM_MOVEABLE, sizeOfRes);
	heapMem = GlobalLock(heapGlobal);
	RtlCopyMemory(heapMem, resData, sizeOfRes);

	if (CreateStreamOnHGlobal(heapMem, TRUE, &pStream) == S_OK) {
		pImg = Image::FromStream(pStream);
		if (!pImg) bResult = false;
	} else
		bResult = false;

	pStream->Release();
	GlobalUnlock(heapGlobal);
	heapMem = GlobalFree(heapGlobal);
	
	if (pImg) this->m_gImage = pImg;
	
    return bResult;	
}

// Destroy this picture
void CPicture::DestroyPicture() {
	if (m_gImage) delete m_gImage;
	m_gImage = NULL;
}

// Get size of this picture
CSize CPicture::GetSize() {
	CSize retSize(0,0);
	if (!m_gImage) return retSize;
	retSize.cx = (LONG)m_gImage->GetWidth();
	retSize.cy = (LONG)m_gImage->GetHeight();
	return retSize;
}


// Draw this picutre on a device Context
void CPicture::Draw(CDC * pDc, CPoint pos, CSize size) {
	Graphics * gr = NULL;
	if (!pDc || !m_gImage) return;

	gr = Graphics::FromHDC(pDc->GetHDC());
	if (size.cx && size.cy)
		gr->DrawImage(m_gImage, pos.x, pos.y, size.cx, size.cy);
	else
		gr->DrawImage(m_gImage, pos.x, pos.y);
}

// Make an HBITMAP transparent
HBITMAP CPicture::MakeBitMapTransparent(HBITMAP hbmSrc, COLORREF clrBk)
{
	HDC hdcSrc = NULL, hdcDst = NULL;
	HBITMAP hbmOld = NULL, hbmNew = NULL;
	BITMAP bm = {0};
	COLORREF clrTP;

	if ((hdcSrc = CreateCompatibleDC(NULL)) != NULL) {
		if ((hdcDst = CreateCompatibleDC(NULL)) != NULL) {
			int nRow, nCol;
			GetObject(hbmSrc, sizeof(bm), &bm);
			hbmOld = (HBITMAP)SelectObject(hdcSrc, hbmSrc);
			hbmNew = CreateBitmap(bm.bmWidth, bm.bmHeight, bm.bmPlanes, bm.bmBitsPixel, NULL);
			SelectObject(hdcDst, hbmNew);
			
			BitBlt(hdcDst,0,0,bm.bmWidth, bm.bmHeight,hdcSrc,0,0,SRCCOPY);

			clrTP = GetPixel(hdcDst, 0, 0);// Get color of first pixel at 0,0
			//clrTP = RGB(255,255,255);
			//clrBK = GetSysColor(COLOR_MENU);// Get the current background color of the menu
			
			for (nRow = 0; nRow < bm.bmHeight; nRow++)// work our way through all the pixels changing their color
				for (nCol = 0; nCol < bm.bmWidth; nCol++)// when we hit our set transparency color.
					if (GetPixel(hdcDst, nCol, nRow) == clrTP)
						SetPixel(hdcDst, nCol, nRow, clrBk);
			
			DeleteDC(hdcDst);
		}
		DeleteDC(hdcSrc);
		
	}
	return hbmNew;// return our transformed bitmap.
}


