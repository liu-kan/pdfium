// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_PAGE_CPDF_PAGEMODULE_H_
#define CORE_FPDFAPI_PAGE_CPDF_PAGEMODULE_H_

#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fxcrt/retain_ptr.h"

class CPDF_Document;
class CPDF_ColorSpace;
class CPDF_DeviceCS;
class CPDF_PatternCS;

class CPDF_PageModule {
 public:
  CPDF_PageModule();
  ~CPDF_PageModule();

  CPDF_FontGlobals* GetFontGlobals();
  RetainPtr<CPDF_ColorSpace> GetStockCS(int family);
  void ClearStockFont(CPDF_Document* pDoc);

 private:
  CPDF_FontGlobals m_FontGlobals;
  RetainPtr<CPDF_DeviceCS> m_StockGrayCS;
  RetainPtr<CPDF_DeviceCS> m_StockRGBCS;
  RetainPtr<CPDF_DeviceCS> m_StockCMYKCS;
  RetainPtr<CPDF_PatternCS> m_StockPatternCS;
};

#endif  // CORE_FPDFAPI_PAGE_CPDF_PAGEMODULE_H_
