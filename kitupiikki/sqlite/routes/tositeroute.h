/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef TOSITEROUTE_H
#define TOSITEROUTE_H

#include "../sqliteroute.h"

class TositeRoute : public SQLiteRoute
{
public:
    TositeRoute(SQLiteModel *model);

    QVariant get(const QString &polku, const QUrlQuery &urlquery) override;
    QVariant post(const QString &polku, const QVariant &data) override;
    QVariant put(const QString &polku, const QVariant &data) override;
    QVariant patch(const QString &polku, const QVariant &data) override;

protected:
    int lisaaTaiPaivita(const QVariant pyynto, int tositeid = 0);

    QVariant hae(int tositeId);

    static QString viite(const QString& numero);
};

#endif // TOSITEROUTE_H