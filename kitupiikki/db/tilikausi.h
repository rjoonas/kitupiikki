/*
   Copyright (C) 2017 Arto Hyvättinen

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

#ifndef TILIKAUSI_H
#define TILIKAUSI_H

#include <QDate>

#include "jsonkentta.h"

/**
 * @brief Yhden tilikauden tiedot
 */
class Tilikausi
{
public:
    Tilikausi();
    Tilikausi(QDate tkalkaa, QDate tkpaattyy, QByteArray json = QByteArray());

    QDate alkaa() const { return alkaa_; }
    QDate paattyy() const { return paattyy_; }

    QString kausivaliTekstina() const;

    JsonKentta *json() { return &json_; }

    /**
     * @brief Tilikauden yli/alijäämä
     * @return Tulos sentteinä
     */
    int tulos() const;

protected:
    QDate alkaa_;
    QDate paattyy_;

    JsonKentta json_;
};

#endif // TILIKAUSI_H