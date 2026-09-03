/****************************************************************************
** Meta object code from reading C++ file 'StationDetailPage.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/StationDetailPage.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StationDetailPage.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_StationDetailPage_t {
    uint offsetsAndSizes[32];
    char stringdata0[18];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[12];
    char stringdata4[8];
    char stringdata5[12];
    char stringdata6[12];
    char stringdata7[14];
    char stringdata8[20];
    char stringdata9[20];
    char stringdata10[21];
    char stringdata11[16];
    char stringdata12[8];
    char stringdata13[18];
    char stringdata14[18];
    char stringdata15[16];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_StationDetailPage_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_StationDetailPage_t qt_meta_stringdata_StationDetailPage = {
    {
        QT_MOC_LITERAL(0, 17),  // "StationDetailPage"
        QT_MOC_LITERAL(18, 15),  // "chargerSelected"
        QT_MOC_LITERAL(34, 0),  // ""
        QT_MOC_LITERAL(35, 11),  // "ChargerInfo"
        QT_MOC_LITERAL(47, 7),  // "charger"
        QT_MOC_LITERAL(55, 11),  // "stationName"
        QT_MOC_LITERAL(67, 11),  // "pricePerKwh"
        QT_MOC_LITERAL(79, 13),  // "backRequested"
        QT_MOC_LITERAL(93, 19),  // "navigationRequested"
        QT_MOC_LITERAL(113, 19),  // "destinationLatitude"
        QT_MOC_LITERAL(133, 20),  // "destinationLongitude"
        QT_MOC_LITERAL(154, 15),  // "destinationName"
        QT_MOC_LITERAL(170, 7),  // "walking"
        QT_MOC_LITERAL(178, 17),  // "onNavigateClicked"
        QT_MOC_LITERAL(196, 17),  // "onFavoriteClicked"
        QT_MOC_LITERAL(214, 15)   // "onFilterClicked"
    },
    "StationDetailPage",
    "chargerSelected",
    "",
    "ChargerInfo",
    "charger",
    "stationName",
    "pricePerKwh",
    "backRequested",
    "navigationRequested",
    "destinationLatitude",
    "destinationLongitude",
    "destinationName",
    "walking",
    "onNavigateClicked",
    "onFavoriteClicked",
    "onFilterClicked"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_StationDetailPage[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    3,   50,    2, 0x06,    1 /* Public */,
       7,    0,   57,    2, 0x06,    5 /* Public */,
       8,    4,   58,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      13,    0,   67,    2, 0x08,   11 /* Private */,
      14,    0,   68,    2, 0x08,   12 /* Private */,
      15,    0,   69,    2, 0x08,   13 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::QString, QMetaType::Double,    4,    5,    6,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::QString, QMetaType::Bool,    9,   10,   11,   12,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject StationDetailPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_StationDetailPage.offsetsAndSizes,
    qt_meta_data_StationDetailPage,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_StationDetailPage_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<StationDetailPage, std::true_type>,
        // method 'chargerSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const ChargerInfo &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'backRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'navigationRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onNavigateClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFavoriteClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFilterClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void StationDetailPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StationDetailPage *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->chargerSelected((*reinterpret_cast< std::add_pointer_t<ChargerInfo>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3]))); break;
        case 1: _t->backRequested(); break;
        case 2: _t->navigationRequested((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 3: _t->onNavigateClicked(); break;
        case 4: _t->onFavoriteClicked(); break;
        case 5: _t->onFilterClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StationDetailPage::*)(const ChargerInfo & , const QString & , double );
            if (_t _q_method = &StationDetailPage::chargerSelected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StationDetailPage::*)();
            if (_t _q_method = &StationDetailPage::backRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (StationDetailPage::*)(double , double , const QString & , bool );
            if (_t _q_method = &StationDetailPage::navigationRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *StationDetailPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StationDetailPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StationDetailPage.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int StationDetailPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void StationDetailPage::chargerSelected(const ChargerInfo & _t1, const QString & _t2, double _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StationDetailPage::backRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void StationDetailPage::navigationRequested(double _t1, double _t2, const QString & _t3, bool _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
