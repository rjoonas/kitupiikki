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

#include "vientimodel.h"

VientiModel::VientiModel()
{

}

int VientiModel::rowCount(const QModelIndex &parent) const
{
    return viennit.count();
}

int VientiModel::columnCount(const QModelIndex &parent) const
{
    return 7;
}

QVariant VientiModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role == Qt::TextAlignmentRole)
        return QVariant( Qt::AlignCenter | Qt::AlignVCenter);
    else if( role != Qt::DisplayRole )
        return QVariant();
    else if( orientation == Qt::Horizontal)
    {
        switch (section)
        {
            case PVM:
                return QVariant("Pvm");
            case TILI:
                return QVariant("Tili");
            case DEBET :
                return QVariant("Debet");
            case KREDIT:
                return QVariant("Kredit");
            case SELITE:
                return QVariant("Selite");
            case KUSTANNUSPAIKKA :
                return QVariant("Kustannuspaikka");
            case PROJEKTI:
                return QVariant("Projekti");
        }

    }
    return QVariant( section + 1);
}

QVariant VientiModel::data(const QModelIndex &index, int role) const
{
    if( !index.isValid())
        return QVariant();
    if( role==Qt::DisplayRole )
    {
        return QVariant("NNNN");
    }
    return QVariant();
}
