/****************************************************************************
** Meta object code from reading C++ file 'HomePage.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/HomePage.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'HomePage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_HomePage_t {
    uint offsetsAndSizes[34];
    char stringdata0[9];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[10];
    char stringdata4[18];
    char stringdata5[18];
    char stringdata6[16];
    char stringdata7[17];
    char stringdata8[16];
    char stringdata9[14];
    char stringdata10[16];
    char stringdata11[11];
    char stringdata12[9];
    char stringdata13[10];
    char stringdata14[8];
    char stringdata15[15];
    char stringdata16[8];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_HomePage_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_HomePage_t qt_meta_stringdata_HomePage = {
    {
        QT_MOC_LITERAL(0, 8),  // "HomePage"
        QT_MOC_LITERAL(9, 15),  // "stationSelected"
        QT_MOC_LITERAL(25, 0),  // ""
        QT_MOC_LITERAL(26, 9),  // "stationId"
        QT_MOC_LITERAL(36, 17),  // "stationsRequested"
        QT_MOC_LITERAL(54, 17),  // "chargingRequested"
        QT_MOC_LITERAL(72, 15),  // "ordersRequested"
        QT_MOC_LITERAL(88, 16),  // "profileRequested"
        QT_MOC_LITERAL(105, 15),  // "onSearchClicked"
        QT_MOC_LITERAL(121, 13),  // "onSortClicked"
        QT_MOC_LITERAL(135, 15),  // "onLocateClicked"
        QT_MOC_LITERAL(151, 10),  // "onGeocoded"
        QT_MOC_LITERAL(162, 8),  // "latitude"
        QT_MOC_LITERAL(171, 9),  // "longitude"
        QT_MOC_LITERAL(181, 7),  // "address"
        QT_MOC_LITERAL(189, 14),  // "onGeocodeError"
        QT_MOC_LITERAL(204, 7)   // "message"
    },
    "HomePage",
    "stationSelected",
    "",
    "stationId",
    "stationsRequested",
    "chargingRequested",
    "ordersRequested",
    "profileRequested",
    "onSearchClicked",
    "onSortClicked",
    "onLocateClicked",
    "onGeocoded",
    "latitude",
    "longitude",
    "address",
    "onGeocodeError",
    "message"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_HomePage[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   74,    2, 0x06,    1 /* Public */,
       4,    0,   77,    2, 0x06,    3 /* Public */,
       5,    0,   78,    2, 0x06,    4 /* Public */,
       6,    0,   79,    2, 0x06,    5 /* Public */,
       7,    0,   80,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,   81,    2, 0x08,    7 /* Private */,
       9,    0,   82,    2, 0x08,    8 /* Private */,
      10,    0,   83,    2, 0x08,    9 /* Private */,
      11,    3,   84,    2, 0x08,   10 /* Private */,
      15,    1,   91,    2, 0x08,   14 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::LongLong,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::QString,   12,   13,   14,
    QMetaType::Void, QMetaType::QString,   16,

       0        // eod
};

Q_CONSTINIT const QMetaObject HomePage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_HomePage.offsetsAndSizes,
    qt_meta_data_HomePage,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_HomePage_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<HomePage, std::true_type>,
        // method 'stationSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'stationsRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'chargingRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'ordersRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'profileRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSearchClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSortClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLocateClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGeocoded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onGeocodeError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void HomePage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<HomePage *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->stationSelected((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 1: _t->stationsRequested(); break;
        case 2: _t->chargingRequested(); break;
        case 3: _t->ordersRequested(); break;
        case 4: _t->profileRequested(); break;
        case 5: _t->onSearchClicked(); break;
        case 6: _t->onSortClicked(); break;
        case 7: _t->onLocateClicked(); break;
        case 8: _t->onGeocoded((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 9: _t->onGeocodeError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (HomePage::*)(qint64 );
            if (_t _q_method = &HomePage::stationSelected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (HomePage::*)();
            if (_t _q_method = &HomePage::stationsRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (HomePage::*)();
            if (_t _q_method = &HomePage::chargingRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (HomePage::*)();
            if (_t _q_method = &HomePage::ordersRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (HomePage::*)();
            if (_t _q_method = &HomePage::profileRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *HomePage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *HomePage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_HomePage.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int HomePage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void HomePage::stationSelected(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void HomePage::stationsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void HomePage::chargingRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void HomePage::ordersRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void HomePage::profileRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
