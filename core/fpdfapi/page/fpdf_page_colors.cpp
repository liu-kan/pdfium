// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/page/pageint.h"

#include <limits.h>

#include <algorithm>

#include "core/fpdfapi/cpdf_modulemgr.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcodec/fx_codec.h"
#include "third_party/base/stl_util.h"

namespace {

float NormalizeChannel(float fVal) {
  return pdfium::clamp(fVal, 0.0f, 1.0f);
}

bool DetectSRGB(const uint8_t* pData, uint32_t dwSize) {
  return dwSize == 3144 && memcmp(pData + 0x190, "sRGB IEC61966-2.1", 17) == 0;
}

}  // namespace

uint32_t ComponentsForFamily(int family) {
  if (family == PDFCS_DEVICERGB)
    return 3;
  if (family == PDFCS_DEVICEGRAY)
    return 1;
  ASSERT(family == PDFCS_DEVICECMYK);
  return 4;
}

void sRGB_to_AdobeCMYK(float R,
                       float G,
                       float B,
                       float& c,
                       float& m,
                       float& y,
                       float& k) {
  c = 1.0f - R;
  m = 1.0f - G;
  y = 1.0f - B;
  k = std::min(c, std::min(m, y));
}

void ReverseRGB(uint8_t* pDestBuf, const uint8_t* pSrcBuf, int pixels) {
  if (pDestBuf == pSrcBuf) {
    for (int i = 0; i < pixels; i++) {
      uint8_t temp = pDestBuf[2];
      pDestBuf[2] = pDestBuf[0];
      pDestBuf[0] = temp;
      pDestBuf += 3;
    }
  } else {
    for (int i = 0; i < pixels; i++) {
      *pDestBuf++ = pSrcBuf[2];
      *pDestBuf++ = pSrcBuf[1];
      *pDestBuf++ = pSrcBuf[0];
      pSrcBuf += 3;
    }
  }
}

CPDF_DeviceCS::CPDF_DeviceCS(CPDF_Document* pDoc, int family)
    : CPDF_ColorSpace(pDoc, family, ComponentsForFamily(family)) {
  ASSERT(family == PDFCS_DEVICEGRAY || family == PDFCS_DEVICERGB ||
         family == PDFCS_DEVICECMYK);
}

CPDF_DeviceCS::~CPDF_DeviceCS() {}

bool CPDF_DeviceCS::GetRGB(float* pBuf, float* R, float* G, float* B) const {
  switch (m_Family) {
    case PDFCS_DEVICEGRAY:
      *R = NormalizeChannel(*pBuf);
      *G = *R;
      *B = *R;
      return true;
    case PDFCS_DEVICERGB:
      *R = NormalizeChannel(pBuf[0]);
      *G = NormalizeChannel(pBuf[1]);
      *B = NormalizeChannel(pBuf[2]);
      return true;
    case PDFCS_DEVICECMYK:
      if (m_dwStdConversion) {
        float k = pBuf[3];
        *R = 1.0f - std::min(1.0f, pBuf[0] + k);
        *G = 1.0f - std::min(1.0f, pBuf[1] + k);
        *B = 1.0f - std::min(1.0f, pBuf[2] + k);
      } else {
        AdobeCMYK_to_sRGB(NormalizeChannel(pBuf[0]), NormalizeChannel(pBuf[1]),
                          NormalizeChannel(pBuf[2]), NormalizeChannel(pBuf[3]),
                          *R, *G, *B);
      }
      return true;
    default:
      ASSERT(false);
      return false;
  }
}

bool CPDF_DeviceCS::v_GetCMYK(float* pBuf,
                              float* c,
                              float* m,
                              float* y,
                              float* k) const {
  if (m_Family != PDFCS_DEVICECMYK)
    return false;

  *c = pBuf[0];
  *m = pBuf[1];
  *y = pBuf[2];
  *k = pBuf[3];
  return true;
}

bool CPDF_DeviceCS::SetRGB(float* pBuf, float R, float G, float B) const {
  switch (m_Family) {
    case PDFCS_DEVICEGRAY:
      if (R != G || R != B)
        return false;
      *pBuf = R;
      return true;
    case PDFCS_DEVICERGB:
      pBuf[0] = R;
      pBuf[1] = G;
      pBuf[2] = B;
      return true;
    case PDFCS_DEVICECMYK:
      sRGB_to_AdobeCMYK(R, G, B, pBuf[0], pBuf[1], pBuf[2], pBuf[3]);
      return true;
    default:
      ASSERT(false);
      return false;
  }
}

bool CPDF_DeviceCS::v_SetCMYK(float* pBuf,
                              float c,
                              float m,
                              float y,
                              float k) const {
  switch (m_Family) {
    case PDFCS_DEVICEGRAY:
      return false;
    case PDFCS_DEVICERGB:
      AdobeCMYK_to_sRGB(c, m, y, k, pBuf[0], pBuf[1], pBuf[2]);
      return true;
    case PDFCS_DEVICECMYK:
      pBuf[0] = c;
      pBuf[1] = m;
      pBuf[2] = y;
      pBuf[3] = k;
      return true;
    default:
      ASSERT(false);
      return false;
  }
}

void CPDF_DeviceCS::TranslateImageLine(uint8_t* pDestBuf,
                                       const uint8_t* pSrcBuf,
                                       int pixels,
                                       int image_width,
                                       int image_height,
                                       bool bTransMask) const {
  switch (m_Family) {
    case PDFCS_DEVICEGRAY:
      for (int i = 0; i < pixels; i++) {
        *pDestBuf++ = pSrcBuf[i];
        *pDestBuf++ = pSrcBuf[i];
        *pDestBuf++ = pSrcBuf[i];
      }
      break;
    case PDFCS_DEVICERGB:
      ReverseRGB(pDestBuf, pSrcBuf, pixels);
      break;
    case PDFCS_DEVICECMYK:
      if (bTransMask) {
        for (int i = 0; i < pixels; i++) {
          int k = 255 - pSrcBuf[3];
          pDestBuf[0] = ((255 - pSrcBuf[0]) * k) / 255;
          pDestBuf[1] = ((255 - pSrcBuf[1]) * k) / 255;
          pDestBuf[2] = ((255 - pSrcBuf[2]) * k) / 255;
          pDestBuf += 3;
          pSrcBuf += 4;
        }
      } else {
        for (int i = 0; i < pixels; i++) {
          if (m_dwStdConversion) {
            uint8_t k = pSrcBuf[3];
            pDestBuf[2] = 255 - std::min(255, pSrcBuf[0] + k);
            pDestBuf[1] = 255 - std::min(255, pSrcBuf[1] + k);
            pDestBuf[0] = 255 - std::min(255, pSrcBuf[2] + k);
          } else {
            AdobeCMYK_to_sRGB1(pSrcBuf[0], pSrcBuf[1], pSrcBuf[2], pSrcBuf[3],
                               pDestBuf[2], pDestBuf[1], pDestBuf[0]);
          }
          pSrcBuf += 4;
          pDestBuf += 3;
        }
      }
      break;
    default:
      ASSERT(false);
      break;
  }
}

CPDF_IccProfile::CPDF_IccProfile(const uint8_t* pData, uint32_t dwSize)
    : m_bsRGB(DetectSRGB(pData, dwSize)) {
  if (m_bsRGB) {
    m_nSrcComponents = 3;
    return;
  }
  auto* pIccModule = CPDF_ModuleMgr::Get()->GetIccModule();
  if (pIccModule) {
    uint32_t nSrcComps = 0;
    m_pTransform = pIccModule->CreateTransform_sRGB(pData, dwSize, nSrcComps);
    if (m_pTransform)
      m_nSrcComponents = nSrcComps;
  }
}

CPDF_IccProfile::~CPDF_IccProfile() {
  if (m_pTransform) {
    CPDF_ModuleMgr::Get()->GetIccModule()->DestroyTransform(m_pTransform);
  }
}
