// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CORE_FWL_NOTEIMP_H_
#define XFA_FWL_CORE_FWL_NOTEIMP_H_

#include "xfa/fwl/core/cfwl_event.h"
#include "xfa/fwl/core/cfwl_message.h"
#include "xfa/fwl/core/fwl_error.h"
#include "xfa/fwl/core/ifwl_widget.h"
#include "xfa/fxgraphics/include/cfx_graphics.h"

#define FWL_KEYFLAG_Ctrl (1 << 0)
#define FWL_KEYFLAG_Alt (1 << 1)
#define FWL_KEYFLAG_Shift (1 << 2)
#define FWL_KEYFLAG_Command (1 << 3)
#define FWL_KEYFLAG_LButton (1 << 4)
#define FWL_KEYFLAG_RButton (1 << 5)
#define FWL_KEYFLAG_MButton (1 << 6)

class CFWL_CoreToolTipDP;
class CFWL_MsgActivate;
class CFWL_MsgDeactivate;
class CFWL_MsgDropFiles;
class CFWL_MsgKey;
class CFWL_MsgKillFocus;
class CFWL_MsgMouse;
class CFWL_MsgMouseWheel;
class CFWL_MsgSetFocus;
class CFWL_MsgSize;
class CFWL_MsgWindowMove;
class CFWL_TargetImp;
class CFWL_ToolTipImp;
class CFWL_WidgetImp;
class IFWL_ToolTipTarget;

class CFWL_NoteLoop {
 public:
  CFWL_NoteLoop(CFWL_WidgetImp* pForm = nullptr);
  ~CFWL_NoteLoop() {}

  FWL_ERR Idle(int32_t count);
  CFWL_WidgetImp* GetForm();
  FX_BOOL ContinueModal();
  FWL_ERR EndModalLoop();
  FWL_ERR SetMainForm(CFWL_WidgetImp* pForm);

 protected:
  void GenerateCommondEvent(uint32_t dwCommand);

  CFWL_WidgetImp* m_pForm;
  FX_BOOL m_bContinueModal;
};

class CFWL_NoteDriver {
 public:
  CFWL_NoteDriver();
  ~CFWL_NoteDriver();

  FX_BOOL SendEvent(CFWL_Event* pNote);
  FWL_ERR RegisterEventTarget(IFWL_Widget* pListener,
                              IFWL_Widget* pEventSource = nullptr,
                              uint32_t dwFilter = FWL_EVENT_ALL_MASK);
  FWL_ERR UnregisterEventTarget(IFWL_Widget* pListener);
  void ClearEventTargets(FX_BOOL bRemoveAll);
  IFWL_App* GetOwnerApp() const;
  FWL_ERR PushNoteLoop(CFWL_NoteLoop* pNoteLoop);
  CFWL_NoteLoop* PopNoteLoop();
  IFWL_Widget* GetFocus();
  FX_BOOL SetFocus(IFWL_Widget* pFocus, FX_BOOL bNotify = FALSE);
  void SetGrab(IFWL_Widget* pGrab, FX_BOOL bSet);
  FWL_ERR Run();

  IFWL_Widget* GetHover();
  void SetHover(IFWL_Widget* pHover);
  void NotifyTargetHide(IFWL_Widget* pNoteTarget);
  void NotifyTargetDestroy(IFWL_Widget* pNoteTarget);
  FWL_ERR RegisterForm(CFWL_WidgetImp* pForm);
  FWL_ERR UnRegisterForm(CFWL_WidgetImp* pForm);
  FX_BOOL QueueMessage(CFWL_Message* pMessage);
  FX_BOOL UnqueueMessage(CFWL_NoteLoop* pNoteLoop);
  CFWL_NoteLoop* GetTopLoop();
  int32_t CountLoop();
  FX_BOOL ProcessMessage(CFWL_Message* pMessage);

 protected:
  FX_BOOL DispatchMessage(CFWL_Message* pMessage, IFWL_Widget* pMessageForm);
  FX_BOOL DoActivate(CFWL_MsgActivate* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoDeactivate(CFWL_MsgDeactivate* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoSetFocus(CFWL_MsgSetFocus* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoKillFocus(CFWL_MsgKillFocus* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoKey(CFWL_MsgKey* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoMouse(CFWL_MsgMouse* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoWheel(CFWL_MsgMouseWheel* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoSize(CFWL_MsgSize* pMsg);
  FX_BOOL DoWindowMove(CFWL_MsgWindowMove* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoDragFiles(CFWL_MsgDropFiles* pMsg, IFWL_Widget* pMessageForm);
  FX_BOOL DoMouseEx(CFWL_MsgMouse* pMsg, IFWL_Widget* pMessageForm);
  void MouseSecondary(CFWL_MsgMouse* pMsg);
  FX_BOOL IsValidMessage(CFWL_Message* pMessage);
  IFWL_Widget* GetMessageForm(IFWL_Widget* pDstTarget);
  void ClearInvalidEventTargets(FX_BOOL bRemoveAll);

  CFX_ArrayTemplate<CFWL_WidgetImp*> m_forms;
  CFX_ArrayTemplate<CFWL_Message*> m_noteQueue;
  CFX_ArrayTemplate<CFWL_NoteLoop*> m_noteLoopQueue;
  CFX_MapPtrToPtr m_eventTargets;
  int32_t m_sendEventCalled;
  IFWL_Widget* m_pHover;
  IFWL_Widget* m_pFocus;
  IFWL_Widget* m_pGrab;
  CFWL_NoteLoop* m_pNoteLoop;
};

class CFWL_EventTarget {
 public:
  CFWL_EventTarget(CFWL_NoteDriver* pNoteDriver, IFWL_Widget* pListener)
      : m_pListener(pListener), m_pNoteDriver(pNoteDriver), m_bInvalid(FALSE) {}
  ~CFWL_EventTarget();
  int32_t SetEventSource(IFWL_Widget* pSource,
                         uint32_t dwFilter = FWL_EVENT_ALL_MASK);
  FX_BOOL ProcessEvent(CFWL_Event* pEvent);
  FX_BOOL IsFilterEvent(CFWL_Event* pEvent, uint32_t dwFilter);
  FX_BOOL IsInvalid() { return m_bInvalid; }
  void FlagInvalid() { m_bInvalid = TRUE; }

 protected:
  CFX_MapPtrTemplate<void*, uint32_t> m_eventSources;
  IFWL_Widget* m_pListener;
  CFWL_NoteDriver* m_pNoteDriver;
  FX_BOOL m_bInvalid;
};

class CFWL_ToolTipContainer {
 public:
  static CFWL_ToolTipContainer* getInstance();
  static void DeleteInstance();

  FX_ERR AddToolTipTarget(IFWL_ToolTipTarget* pTarget);
  FX_ERR RemoveToolTipTarget(IFWL_ToolTipTarget* pTarget);
  IFWL_ToolTipTarget* GetCurrentToolTipTarget();

  FX_BOOL HasToolTip(IFWL_Widget* pWidget);

  FX_BOOL ProcessEnter(CFWL_EvtMouse* pEvt, IFWL_Widget* pOwner);
  FX_BOOL ProcessLeave(CFWL_EvtMouse* pEvt);

 protected:
  CFWL_ToolTipContainer();
  virtual ~CFWL_ToolTipContainer();

  IFWL_ToolTipTarget* pCurTarget;
  CFWL_ToolTipImp* m_pToolTipImp;
  CFWL_CoreToolTipDP* m_ToolTipDp;
  CFX_ArrayTemplate<IFWL_ToolTipTarget*> m_arrWidget;

 private:
  static CFWL_ToolTipContainer* s_pInstance;
};

#endif  // XFA_FWL_CORE_FWL_NOTEIMP_H_
