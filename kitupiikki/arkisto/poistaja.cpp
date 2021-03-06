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

#include <cmath>

#include "poistaja.h"
#include "ui_poistaja.h"

#include "kirjaus/ehdotusmodel.h"
#include "raportti/raportinkirjoittaja.h"

#include <QSqlQuery>
#include <QDebug>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QPrinter>
#include <QPainter>
#include <QSqlError>

Poistaja::Poistaja(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Poistaja)
{
    ui->setupUi(this);
}

Poistaja::~Poistaja()
{
    delete ui;
}

bool Poistaja::teeSumuPoistot(Tilikausi kausi)
{
    Poistaja poistoDlg;
    return poistoDlg.sumupoistaja(kausi);
}

bool Poistaja::sumupoistaja(Tilikausi kausi)
{
    EhdotusModel ehdotus;
    RaportinKirjoittaja kirjoittaja;

    kirjoittaja.asetaOtsikko("POISTOLASKELMA");
    kirjoittaja.asetaKausiteksti( kausi.paattyy().toString("dd.MM.yyyy"));

    kirjoittaja.lisaaSarake("TUN123456/31.12.9999");
    kirjoittaja.lisaaVenyvaSarake();
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaSarake("Sääntö");
    kirjoittaja.lisaaEurosarake();
    kirjoittaja.lisaaEurosarake();

    RaporttiRivi otsikko;
    otsikko.lisaa("Tili/Pvm");
    otsikko.lisaa("Nimike");
    otsikko.lisaa("Saldo ennen",1, true);
    otsikko.lisaa("Poistosääntö", 1, true);
    otsikko.lisaa("Poisto",1,true);
    otsikko.lisaa("Saldo jälkeen",1,true);
    kirjoittaja.lisaaOtsake(otsikko);


    for( int i=0; i < kp()->tilit()->rowCount(QModelIndex()); i++)
    {
        Tili tili = kp()->tilit()->tiliIndeksilla(i);
        if( !tili.onko(TiliLaji::POISTETTAVA))
            continue;

        qlonglong saldo = tili.saldoPaivalle( kausi.paattyy());

        if( !saldo)
            continue;

        if( !kp()->tilit()->tiliNumerolla( tili.json()->luku("Poistotili") ).onkoValidi() )
        {
            QMessageBox::critical(nullptr, tr("Kitupiikin virhe"),
                                  tr("Poistoja ei voi kirjata, koska tilille %1 ei ole määritelty "
                                     "kelvollista poistojen kirjaustiliä.").arg(tili.numero()));
            return false;
        }

        if( tili.onko(TiliLaji::MENOJAANNOSPOISTO))
        {
            //  Menojäännöspoistossa poistetaan määrätty prosenttimäärä siihen asti olevasta saldosta

            int poistoprosentti = tili.json()->luku("Menojaannospoisto");
            qlonglong poisto = std::round( saldo * poistoprosentti / 100.0 );
            qlonglong jalkeen = saldo - poisto;

            RaporttiRivi rr;
            rr.lisaaLinkilla(RaporttiRiviSarake::TILI_LINKKI, tili.id(), QString::number(tili.numero()) );
            rr.lisaa(tili.nimi());
            rr.lisaa(saldo);
            rr.lisaa( tr("%1 %").arg( poistoprosentti ), 1, true);
            rr.lisaa( poisto );
            rr.lisaa( jalkeen );
            rr.lihavoi();

            kirjoittaja.lisaaRivi(rr);

            // #123: Jos poistotili käsitellään kohdennuksilla, kirjataan poistotkin vastaaviin kohdennuksiin
            // jaettuina

            if( tili.json()->luku("Kohdennukset"))
            {
                QSqlQuery kohkysely( QString("SELECT kohdennus, SUM(debetsnt), SUM(kreditsnt) FROM vienti WHERE "
                                             "tili=%1 AND pvm < '%2' GROUP BY kohdennus ORDER BY kohdennus" )
                                     .arg(tili.id()).arg(kausi.paattyy().toString(Qt::ISODate)) );
                while( kohkysely.next())
                {
                    Kohdennus kohdennus = kp()->kohdennukset()->kohdennus(kohkysely.value(0).toInt());
                    qlonglong kohdsaldo = kohkysely.value(1).toLongLong() - kohkysely.value(2).toLongLong();
                    qlonglong kohdpoisto = std::round( kohdsaldo * poistoprosentti / 100.0 );
                    qlonglong kohdjalkeen = kohdsaldo - kohdpoisto;

                    RaporttiRivi kr;
                    kr.lisaa("");
                    kr.lisaa( kohdennus.nimi() );
                    kr.lisaa( kohdsaldo );
                    kr.lisaa( tr("%1 %").arg( poistoprosentti ), 1, true);
                    kr.lisaa( kohdpoisto);
                    kr.lisaa( kohdjalkeen);
                    kirjoittaja.lisaaRivi( kr);

                    VientiRivi rivi;
                    rivi.pvm = kausi.paattyy();
                    rivi.tili = tili;
                    rivi.kreditSnt = kohdpoisto;
                    rivi.selite = tr("Menojäännöspoisto %1 % %4 saldo ennen %L2 €, jälkeen %L3 €")
                            .arg(poistoprosentti)
                            .arg(kohdsaldo / 100.0,0, 'f',2)
                            .arg(kohdjalkeen / 100.0,0, 'f',2)
                            .arg( kohdennus.nimi());
                    rivi.kohdennus = kohdennus;
                    ehdotus.lisaaVienti(rivi);

                    VientiRivi poistotilille;
                    poistotilille.pvm = kausi.paattyy();
                    poistotilille.tili = kp()->tilit()->tiliNumerolla( tili.json()->luku("Poistotili") );
                    poistotilille.debetSnt = kohdpoisto;
                    poistotilille.kohdennus = kohdennus;
                    poistotilille.selite = tr("Tilin %1 %2 %3 menojäännöspoisto")
                            .arg(tili.numero())
                            .arg(tili.nimi())
                            .arg(kohdennus.nimi());

                    ehdotus.lisaaVienti(poistotilille);
                }


            }
            else
            {
                // Tehdään kirjaus
                VientiRivi vienti;
                vienti.pvm = kausi.paattyy();
                vienti.tili = tili;
                vienti.kreditSnt = poisto;
                vienti.selite = tr("Menojäännöspoisto %1 % saldo ennen %L2 €, jälkeen %L3 €")
                        .arg(poistoprosentti)
                        .arg(saldo / 100.0,0, 'f',2)
                        .arg(jalkeen / 100.0,0, 'f',2);
                ehdotus.lisaaVienti(vienti);

                VientiRivi poistotilille;
                poistotilille.pvm = kausi.paattyy();
                poistotilille.tili = kp()->tilit()->tiliNumerolla( tili.json()->luku("Poistotili") );
                poistotilille.debetSnt = poisto;
                poistotilille.selite = tr("Tilin %1 %2 menojäännöspoisto")
                        .arg(tili.numero())
                        .arg(tili.nimi());

                ehdotus.lisaaVienti(poistotilille);
            }
            kirjoittaja.lisaaRivi();


        }
        else if( tili.onko( TiliLaji::TASAERAPOISTO))
        {
            // Tasaeräpoistossa poistetaan tietty kuukausierä
            // Nyt sitten haetaan tilin tase-erät ;)

            RaporttiRivi tiliRivi;
            tiliRivi.lisaaLinkilla(RaporttiRiviSarake::TILI_LINKKI, tili.id(), QString::number(tili.numero()) );
            tiliRivi.lisaa( tili.nimi());
            tiliRivi.lihavoi();
            kirjoittaja.lisaaRivi(tiliRivi);

            EranValintaModel emodel;
            emodel.lataa(tili, false, kausi.paattyy());
            for(int eI=1; eI < emodel.rowCount(QModelIndex()); eI++)
            {
                QModelIndex eInd = emodel.index(eI, 0);

                int eranId = eInd.data(EranValintaModel::EraIdRooli).toInt();
                int eraSaldo = eInd.data(EranValintaModel::SaldoRooli).toInt();
                QDate eranPvm = eInd.data(EranValintaModel::PvmRooli).toDate();

                int alkuSnt = 0;
                int poistoKk = 0;

                QSqlQuery alkuKysely;
                alkuKysely.exec(QString("SELECT debetsnt, kreditsnt, json, kohdennus FROM vienti WHERE id=%1").arg(eranId) );
                if( alkuKysely.next())
                {
                    alkuSnt = alkuKysely.value("debetsnt").toInt() - alkuKysely.value("kreditsnt").toInt();
                    JsonKentta json( alkuKysely.value("json").toByteArray());
                    poistoKk = json.luku("Tasaerapoisto");
                }

                if( !poistoKk)
                    continue;

                // Montako kuukautta on kulunut hankinnasta
                int kuukauttaKulunut = kausi.paattyy().year() * 12 + kausi.paattyy().month() -
                                       eranPvm.year() * 12 - eranPvm.month() + 1;



                // Laskennallinen poisto: Paljonko tähän asti voitaisiin poistaa
                int laskennallinenPoisto = alkuSnt * kuukauttaKulunut / poistoKk ;
                if( laskennallinenPoisto > alkuSnt)
                    laskennallinenPoisto = alkuSnt; // Poistetaan vain se, mitä on jäljellä ...


                int eraPoisto = laskennallinenPoisto - alkuSnt + eraSaldo;

                RaporttiRivi rr;
                rr.lisaa( eranPvm );
                rr.lisaa( eInd.data(EranValintaModel::SeliteRooli).toString());
                rr.lisaa( eraSaldo );
                if( poistoKk % 12)      // Poistoaika
                    rr.lisaa( tr( "%1 v %2 kk").arg(poistoKk / 12).arg(poistoKk % 12), 1, true);
                else
                    rr.lisaa( tr("%1 v").arg(poistoKk / 12), 1, true);
                rr.lisaa( eraPoisto);
                rr.lisaa( eraSaldo - eraPoisto );

                kirjoittaja.lisaaRivi(rr);


                // Tehdään kirjaus
                VientiRivi vienti;
                vienti.pvm = kausi.paattyy();
                vienti.tili = tili;
                vienti.kreditSnt = eraPoisto;
                vienti.eraId = eInd.data(EranValintaModel::EraIdRooli).toInt();
                vienti.selite = tr("Tasaeräpoisto %1 ").arg( eInd.data(EranValintaModel::SeliteRooli).toString() );
                // #123: Kohdennetaan poisto kirjauksen kohdennuksen mukaan
                vienti.kohdennus = kp()->kohdennukset()->kohdennus( alkuKysely.value("kohdennus").toInt() );
                ehdotus.lisaaVienti(vienti);

                VientiRivi poistotilille;
                poistotilille.pvm = kausi.paattyy();
                poistotilille.tili = kp()->tilit()->tiliNumerolla( tili.json()->luku("Poistotili") );
                poistotilille.debetSnt = eraPoisto;
                poistotilille.selite = tr("Tasaeräpoisto %3 tilillä %1 %2")
                        .arg(tili.numero())
                        .arg(tili.nimi())
                        .arg( eInd.data(EranValintaModel::SeliteRooli).toString());
                ehdotus.lisaaVienti(poistotilille);

            }
            kirjoittaja.lisaaRivi();

        }
    }

    // Näytetään ehdotus



    ui->browser->setHtml( kirjoittaja.html());
    if( exec() )
    {
        TositeModel tosite( kp()->tietokanta() );
        tosite.asetaPvm( kausi.paattyy() );
        tosite.asetaTositelaji( 0 );
        tosite.asetaOtsikko( tr("Suunnitelman mukaiset poistot %1 - %2")
                             .arg(kausi.alkaa().toString("dd.MM.yyyy"))
                             .arg(kausi.paattyy().toString("dd.MM.yyyy")));
        tosite.json()->set("Sumupoistot", kausi.paattyy());
        ehdotus.tallenna(tosite.vientiModel());

        // Liitetään laskelma
        QPrinter printer;

        printer.setPageSize(QPrinter::A4);
        QString tpnimi = kp()->tilapainen("sumu-XXXX.pdf");
        printer.setOutputFileName( tpnimi );

        QPainter painter(&printer);

        kirjoittaja.tulosta(&printer, &painter);
        painter.end();

        tosite.liiteModel()->lisaaTiedosto( tpnimi, tr("Poistolaskelma"));

        if( tosite.tallenna() )
        {
            kp()->tilikaudet()->json( kausi.paattyy() )->set("Poistokirjaus", tosite.id());
            kp()->tilikaudet()->tallennaJSON();

            return true;
        }
        else
            QMessageBox::critical(this, tr("Virhe poistotositteen tallentamisessa"),
                                  tr("Poistojen tallentuminen epäonnistui seuraavan "
                                     "tietokantavirheen takia: %1").arg( kp()->viimeVirhe() ));
    }
    return false;
}

bool Poistaja::onkoPoistoja(const Tilikausi& kausi)
{

    QSqlQuery kysely;
    kysely.exec( QString("SELECT sum(debetsnt) as db, sum(kreditsnt) as kr from vienti,tili where vienti.tili = tili.id and "
                         "(tili.tyyppi=\"APM\" or tili.tyyppi=\"APT\") and pvm <= \"%1\" ").arg(kausi.paattyy().toString(Qt::ISODate)) );

    if( kysely.next())
        return kysely.value("db").toInt() != kysely.value("kr").toInt();

    return false;
}
