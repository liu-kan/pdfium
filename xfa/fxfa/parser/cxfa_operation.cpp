// Copyright 2017 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_operation.h"

namespace {

const XFA_Attribute kAttributeData[] = {
    XFA_Attribute::Id,     XFA_Attribute::Name,    XFA_Attribute::Use,
    XFA_Attribute::Output, XFA_Attribute::Usehref, XFA_Attribute::Input,
    XFA_Attribute::Unknown};

constexpr wchar_t kName[] = L"operation";

}  // namespace

CXFA_Operation::CXFA_Operation(CXFA_Document* doc, XFA_XDPPACKET packet)
    : CXFA_Node(doc,
                packet,
                XFA_XDPPACKET_ConnectionSet,
                XFA_ObjectType::TextNode,
                XFA_Element::Operation,
                nullptr,
                kAttributeData,
                kName) {}

CXFA_Operation::~CXFA_Operation() {}