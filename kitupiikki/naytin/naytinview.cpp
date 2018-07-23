/*
   Copyright (C) 2018 Arto Hyvättinen

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
#include "naytinview.h"

#include "pdfscene.h"
#include "kuvanaytin.h"
#include "raporttiscene.h"

NaytinView::NaytinView(QWidget *parent)
    : QGraphicsView(parent)
{

}


void NaytinView::nayta(const QByteArray &data)
{
    if( data.startsWith("%PDF"))
        vaihdaScene( new PdfScene(data, this) );
    else
        vaihdaScene( new KuvaNaytin(data, this));

}

void NaytinView::nayta(RaportinKirjoittaja raportti)
{
    vaihdaScene( new RaporttiScene(raportti)  );
}

void NaytinView::sivunAsetuksetMuuttuneet()
{
    if( scene_->sivunAsetuksetMuuttuneet() )
        paivita();
}

void NaytinView::paivita()
{
    scene_->piirraLeveyteen( width() - 20.0 );
}

QString NaytinView::otsikko() const
{
    return scene_->otsikko();
}

bool NaytinView::csvKaytossa() const
{
    return scene_->csvMuoto();
}

QByteArray NaytinView::csv()
{
    return scene_->csv();
}

QString NaytinView::tiedostonMuoto()
{
    return scene_->tiedostonMuoto();
}

QString NaytinView::tiedostoPaate()
{
    return scene_->tiedostoPaate();
}

QByteArray NaytinView::data()
{
    return scene_->data();
}

void NaytinView::vaihdaScene(NaytinScene *uusi)
{
    if( scene_)
        scene_->deleteLater();

    scene_ = uusi;
    connect( uusi, &NaytinScene::sisaltoVaihtunut, this, &NaytinView::sisaltoVaihtunut );

    emit( sisaltoVaihtunut(scene_->tyyppi()));

    setScene(uusi);
    paivita();
}

void NaytinView::resizeEvent(QResizeEvent * /*event*/)
{
    if( scene_)
        scene_->piirraLeveyteen( width() - 20.0);
}
