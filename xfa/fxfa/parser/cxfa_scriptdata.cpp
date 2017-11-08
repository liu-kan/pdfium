// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_scriptdata.h"

#include "xfa/fxfa/parser/cxfa_node.h"

CXFA_ScriptData::CXFA_ScriptData(CXFA_Node* pNode) : CXFA_Data(pNode) {}

XFA_SCRIPTTYPE CXFA_ScriptData::GetContentType() {
  WideStringView cData;
  if (m_pNode->JSNode()->TryCData(XFA_Attribute::ContentType, cData, false)) {
    if (cData == L"application/x-javascript")
      return XFA_SCRIPTTYPE_Javascript;
    if (cData == L"application/x-formcalc")
      return XFA_SCRIPTTYPE_Formcalc;
    return XFA_SCRIPTTYPE_Unkown;
  }
  return XFA_SCRIPTTYPE_Formcalc;
}

int32_t CXFA_ScriptData::GetRunAt() {
  return m_pNode->JSNode()->GetEnum(XFA_Attribute::RunAt);
}

void CXFA_ScriptData::GetExpression(WideString& wsExpression) {
  m_pNode->JSNode()->TryContent(wsExpression, false, true);
}