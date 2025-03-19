#include "fiowordsequence.h"

#include <library/cpp/charset/codepage.h>

EFIOType TFullFIO::GetType() const
{
    if(MultiWordFio)
        return MWFio;

    bool bF = !!Surname;
    bool bI = !!Name;
    bool bO = !!Patronymic;

    if( bF && bI && bO)
    {
        if( InitialName )
            return FIinOinName;
        else
            if( InitialPatronymic )
                return FIOinName;
        return FIOname;
    }
    if( bF && bI )
    {
        if( InitialName )
            return FIinName;
        return FIname;
    }
    if( bF )
        return Fname;
    if( bI && bO)
        return IOname;
    if( bI )
        return Iname;

    ythrow yexception() << "Bad FIO in \"TFullFIO::GetType()\"";
}

void TFullFIO::ToLower()
{
    Surname = ::ToLower(Surname, csYandex);
    Name = ::ToLower(Name, csYandex);
    Patronymic = ::ToLower(Patronymic, csYandex);
}

TString TFullFIO::ToString() const
{
    TString strRes;
    if( !!Surname )
    {
        strRes += Surname;
        strRes += " ";
    }
    if( !!Name )
    {
        strRes += Name;
        strRes += " ";
    }
    if( !!Patronymic )
    {
        strRes += Patronymic;
        strRes += " ";
    }
    return strRes;

}

void TFullFIO::Reset()
{
    InitialPatronymic = false;
    InitialName = false;
    FoundSurname = false;

    SurnameLemma = false;
    FoundPatronymic = false;
    FirstNameFromMorph = true;
    MultiWordFio = false;
    Name.erase();
    Patronymic.erase();
    Surname.erase();
    LocalSuranamePardigmID = -1;
}

TFullFIO::TFullFIO()
{
    Reset();
}

bool TFullFIO::operator<(const TFullFIO& FIO) const
{
    EFIOType type = GetType();
    if(  type !=  FIO.GetType() )
    {
    TString s;
    sprintf(s, "Sorting by \"operator<\" not homogeneous FIOs! (%s, %s)", ToString().data(), FIO.ToString().data() );
        throw yexception() << s;
    }

    switch( type )
    {
        case FIOname:
        case FIinOinName:
        case FIOinName:
            return order_by_fio(FIO, false);
        case FIname:
        case FIinName:
            return order_by_fi(FIO, false);
        case Fname:
        case MWFio:
            return order_by_f(FIO, false);
        case IOname:
            return order_by_io(FIO, false);
        case Iname:
            return order_by_i(FIO, false);
        default:
            ythrow yexception() << "Bad FIO in \"TFullFIO::operator<\"";
    }
}


bool TFullFIO::Gleiche(const TFullFIO& fio) const
{
    if( !fio.Genders.Empty() &&
        !Genders.Empty() )
        return !(Genders & fio.Genders & NSpike::AllGenders).Empty();

    return true;
}


bool TFullFIO::HasInitialName() const
{
    return InitialName;
}

bool TFullFIO::HasInitialPatronymic() const
{
    return InitialPatronymic;
}


bool TFullFIO::HasInitials() const
{
    return InitialName || InitialPatronymic;
}


bool TFullFIO::IsEmpty() const
{
    return (Surname.length() == 0) &&  (Name.length() == 0) && (Patronymic.length() == 0);
}


bool TFullFIO::equal_by_f(const TFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    return !order_by_f(FIO2, bUseInitials) && !FIO2.order_by_f(*this, bUseInitials);
}

bool TFullFIO::equal_by_i(const TFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    return !order_by_i(FIO2, bUseInitials) && !FIO2.order_by_i(*this, bUseInitials);
}

bool TFullFIO::equal_by_o(const TFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    return !order_by_o(FIO2, bUseInitials) && !FIO2.order_by_o(*this, bUseInitials);
}

bool TFullFIO::order_by_f(const TFullFIO& FIO2, bool /*bUseInitials = true*/) const
{
    //если еще не присвоили m_iLocalSuranamePardigmID, то сравниваем просто лексикографически
    if( (LocalSuranamePardigmID == -1) || (FIO2.LocalSuranamePardigmID == -1) )
        return Surname < FIO2.Surname;
    else
        return LocalSuranamePardigmID < FIO2.LocalSuranamePardigmID;
}

bool TFullFIO::order_by_i(const TFullFIO& FIO2, bool /*bUseInitials = true*/) const
{
    return Name < FIO2.Name;
}

bool TFullFIO::order_by_iIn(const TFullFIO& FIO2) const
{
    if( !Name )
        return true;
    if( !FIO2.Name )
        return false;
    return strncmp(Name.data(), (FIO2.Name).data(), 1) < 0;
}


bool TFullFIO::default_order(const TFullFIO& cluster2) const
{
    return order_by_fio(cluster2, false);
}

bool TFullFIO::order_by_o(const TFullFIO& FIO2, bool /*bUseInitials = true*/) const
{
    return Patronymic < FIO2.Patronymic;
}

bool TFullFIO::order_by_oIn(const TFullFIO& FIO2) const
{
    if( !Patronymic )
        return true;
    if( !FIO2.Patronymic )
        return false;

    return strncmp(Patronymic.data(), FIO2.Patronymic.data(), 1) < 0;
}


bool TFullFIO::order_by_fio(const TFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    bool bLess = order_by_f(FIO2);
    if( bLess || FIO2.order_by_f(*this) )
        return bLess;
    return order_by_io(FIO2, bUseInitials);
}

bool TFullFIO::order_by_fiInoIn(const TFullFIO& FIO2, bool /*bUseInitials = true*/) const
{
    bool bLess = order_by_f(FIO2);
    if( bLess || FIO2.order_by_f(*this) )
        return bLess;

    if( order_by_iIn(FIO2) )
        return true;

    if( FIO2.order_by_iIn(*this) )
        return false;

    if( order_by_oIn(FIO2) )
        return true;

    return false;
}

bool TFullFIO::order_by_fioIn(const TFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    bool bLess = order_by_f(FIO2);
    if( bLess || FIO2.order_by_f(*this) )
        return bLess;

    if( order_by_i(FIO2, bUseInitials) )
        return true;

    if( order_by_oIn(FIO2) )
        return true;

    return false;
}


bool TFullFIO::order_by_io(const TFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    if( Name != FIO2.Name  )
    {
        return order_by_i(FIO2, bUseInitials);
    }
    return order_by_o(FIO2, bUseInitials);
}

bool TFullFIO::order_by_fi(const TFullFIO& FIO2, bool bUseInitials /*= true*/) const
{
    bool bLess = order_by_f(FIO2);
    if( bLess || FIO2.order_by_f(*this) )
        return bLess;
    return order_by_i(FIO2, bUseInitials);
}

bool TFullFIO::order_by_fiIn(const TFullFIO& FIO2, bool /*bUseInitials = true*/) const
{
    bool bLess = order_by_f(FIO2);
    if( bLess || FIO2.order_by_f(*this) )
        return bLess;
    return order_by_iIn(FIO2);
}


bool TFullFIO::operator==(const TFullFIO& FIO) const
{
    bool bEqualSurnames = !order_by_f(FIO) && !FIO.order_by_f(*this);
    return    bEqualSurnames &&
            (Name == FIO.Name) &&
            (Patronymic == FIO.Patronymic) &&
            (FoundPatronymic == FIO.FoundPatronymic);
}


TString TFioWordSequence::GetLemma() const
{
    TString strRes;
    strRes += Fio.Surname;
    strRes += " ";
    strRes += Fio.Name;
    strRes += " ";
    strRes += Fio.Patronymic;

    return strRes;
}

