/*
 * ya_property.c
 * Copyright (C) 2015-2016 Peter Belkner <pbelkner@snafu.de>
 *
 * This file is part of libya.
 *
 * libya is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libya is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libya.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ya.h>

///////////////////////////////////////////////////////////////////////////////
void PropertiesLoad(const Property *pProperty, PropertyIOConfig *c)
{
  while (pProperty->type) {
    pProperty->type->Load(pProperty,c);
    ++pProperty;
  }
}

void PropertiesSave(const Property *pProperty, const PropertyIOConfig *c)
{
  while (pProperty->type) {
    pProperty->type->Save(pProperty,c);
    ++pProperty;
  }
}

///////////////////////////////////////////////////////////////////////////////
void PropertySaveInt(const wchar_t *group, const wchar_t *key, int n,
    const wchar_t *path)
{
  wchar_t buf[YA_PROPERTY_SIZE] = {0};

  _itow_s(n, buf, ARRAYSIZE(buf), 10);

  WritePrivateProfileStringW(
	(group ? group : L"default"),    // _In_  LPCTSTR lpAppName,
    key,      // _In_  LPCTSTR lpKeyName,
    buf,      // _In_  LPCTSTR lpString,
    path      // _In_  LPCTSTR lpFileName
  );
}

static void IntTypeLoad(const Property *pProperty, PropertyIOConfig *c)
{
  PROPERTY_INT(pProperty,c->pData)=GetPrivateProfileIntW(
    (c->group ? c->group : L"default"),         // _In_  LPCTSTR lpAppName,
    pProperty->key,   // _In_  LPCTSTR lpKeyName,
    PROPERTY_INT(pProperty,c->pDefault),
                      // _In_  INT nDefault,
    c->path           // _In_  LPCTSTR lpFileName
  );

  DWPRINTF(0,L"%soption \"%s\": %d (%d)\n",
      c->pfx,pProperty->key,
      PROPERTY_INT(pProperty,c->pData),
      PROPERTY_INT(pProperty,c->pDefault));
}

static void IntTypeSave(const Property *pProperty, const PropertyIOConfig *c)
{
  PropertySaveInt(c->group,pProperty->key,
                  PROPERTY_INT(pProperty,c->pData),
      c->path);
}

const PropertyType gcIntType={
  IntTypeLoad,
  IntTypeSave
};

///////////////////////////////////////////////////////////////////////////////
static void DoubleTypeLoad(const Property *pProperty, PropertyIOConfig *c)
{
  wchar_t set[YA_PROPERTY_SIZE] = {0};
  wchar_t get[YA_PROPERTY_SIZE] = {0};

  // cppcheck-suppress invalidPointerCast
  StringCchPrintf(set,YA_PROPERTY_SIZE,L"%f",PROPERTY_DOUBLE(pProperty,c->pDefault));

  GetPrivateProfileStringW(
    (c->group ? c->group : L"default"),         // _In_   LPCTSTR lpAppName,
    pProperty->key,   // _In_   LPCTSTR lpKeyName,
    set,              // _In_   LPCTSTR lpDefault,
    get,              // _Out_  LPTSTR lpReturnedString,
    YA_PROPERTY_SIZE, // _In_   DWORD nSize,
    c->path           // _In_   LPCTSTR lpFileName
  );

  // cppcheck-suppress invalidPointerCast
  PROPERTY_DOUBLE(pProperty,c->pData)=_wtof(get);
  // cppcheck-suppress invalidPointerCast
  DWPRINTF(0,L"%soption \"%s\": %f (%f)\n",
      c->pfx,pProperty->key,
      PROPERTY_DOUBLE(pProperty,c->pData),
      PROPERTY_DOUBLE(pProperty,c->pDefault));
}

static void DoubleTypeSave(const Property *pProperty,
    const PropertyIOConfig *c)
{
  wchar_t buf[YA_PROPERTY_SIZE] = {0};

  // cppcheck-suppress invalidPointerCast
  StringCchPrintf(buf,YA_PROPERTY_SIZE,L"%f",PROPERTY_DOUBLE(pProperty,c->pData));

  WritePrivateProfileStringW(
    (c->group ? c->group : L"default"),         // _In_  LPCTSTR lpAppName,
    pProperty->key,   // _In_  LPCTSTR lpKeyName,
    buf,              // _In_  LPCTSTR lpString,
    c->path           // _In_  LPCTSTR lpFileName
  );
}

const PropertyType gcDoubleType={
  DoubleTypeLoad,
  DoubleTypeSave
};

///////////////////////////////////////////////////////////////////////////////
static void StringTypeLoad(const Property *pProperty, PropertyIOConfig *c)
{
  GetPrivateProfileStringW(
    (c->group ? c->group : L"default"),         // _In_   LPCTSTR lpAppName,
    pProperty->key,   // _In_   LPCTSTR lpKeyName,
    PROPERTY_STRING(pProperty,c->pDefault),
                      // _In_   LPCTSTR lpDefault,
    PROPERTY_STRING(pProperty,c->pData),
                      // _Out_  LPTSTR lpReturnedString,
    pProperty->size,  // _In_   DWORD nSize,
    c->path           // _In_   LPCTSTR lpFileName
  );

  DWPRINTF(0,L"%soption \"%s\": \"%s\" (\"%s\")\n",
      c->pfx,pProperty->key,
      PROPERTY_STRING(pProperty,c->pData),
      PROPERTY_STRING(pProperty,c->pDefault));
}

static void StringTypeSave(const Property *pProperty,
    const PropertyIOConfig *c)
{
  WritePrivateProfileStringW(
    (c->group ? c->group : L"default"),         // _In_  LPCTSTR lpAppName,
    pProperty->key,   // _In_  LPCTSTR lpKeyName,
    PROPERTY_STRING(pProperty,c->pData),
                      // _In_  LPCTSTR lpString,
    c->path           // _In_  LPCTSTR lpFileName
  );
}

const PropertyType gcStringType={
  StringTypeLoad,
  StringTypeSave
};
