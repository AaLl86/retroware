#pragma once
using namespace Gdiplus;

class CPicture
{
public:
	CPicture(void);
	CPicture(UINT nResId);
	~CPicture(void);

	// Create an Image starting from a Res ID
	bool CreatePicture(UINT nResId = 0);

	// Destroy this picture
	void DestroyPicture();

	// Draw this picutre on a device Context
	void Draw(CDC * dc, CPoint pos, CSize size = CSize(0,0));

	// Get size of this picture
	CSize GetSize();

	// Make an HBITMAP transparent
	static HBITMAP MakeBitMapTransparent(HBITMAP hbmSrc, COLORREF clrBk);

private:
	Image * m_gImage;
};
