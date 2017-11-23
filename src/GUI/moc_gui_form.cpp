/****************************************************************************
** Meta object code from reading C++ file 'gui_form.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.6)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "gui_form.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gui_form.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.6. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_gui_form[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      10,    9,    9,    9, 0x05,
      31,    9,    9,    9, 0x05,

 // slots: signature, parameters, type, tag, flags
      49,    9,    9,    9, 0x08,
      65,    9,    9,    9, 0x08,
      81,    9,    9,    9, 0x08,
     108,    9,    9,    9, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_gui_form[] = {
    "gui_form\0\0mousePressed(QPoint)\0"
    "mouseClickEvent()\0DisableButton()\0"
    "UpdatePicture()\0processFrameAndUpdateGUI()\0"
    "doNextFrame()\0"
};

void gui_form::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        gui_form *_t = static_cast<gui_form *>(_o);
        switch (_id) {
        case 0: _t->mousePressed((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 1: _t->mouseClickEvent(); break;
        case 2: _t->DisableButton(); break;
        case 3: _t->UpdatePicture(); break;
        case 4: _t->processFrameAndUpdateGUI(); break;
        case 5: _t->doNextFrame(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData gui_form::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject gui_form::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_gui_form,
      qt_meta_data_gui_form, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &gui_form::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *gui_form::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *gui_form::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_gui_form))
        return static_cast<void*>(const_cast< gui_form*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int gui_form::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void gui_form::mousePressed(const QPoint & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void gui_form::mouseClickEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
