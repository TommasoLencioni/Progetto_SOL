#!/bin/bash
echo "### INIZIO ANALISI LOG ###"

#CLIENTI USCITI REGOLARMENTE
#C|n prod|tempo nel supermercato|tempo in coda|code
# VARIABILI
clienti=0
clienti_quit=0
prodotti=0
clienti_noprod=0
tot_clienti=0
tempoin=0.000
tempocoda=0.000
cambiocode=0
clienti_cambiocode=0
cat ./log/* | grep -e "C|" | sort > ./log/tmp_clienti
exec 3<./log/tmp_clienti

while read -u 3 riga; do
    echo ${riga}
    clienti=$(($clienti + 1))

    riga=${riga#C|*|} #n prod|tempo nel supermercato|tempo in coda|code
    tmp_prodotti=${riga%%|*} #n prod
    if [ $tmp_prodotti -ne 0 ]; then
        prodotti=$(($prodotti + $tmp_prodotti));
    else
        clienti_noprod=$(($clienti_noprod + 1));
    fi

    riga=${riga#*|} #tempo nel supermercato|tempo in coda|code
    tmp_tempoin=${riga%%|*}  #tempo nel supermercato
    tempoin=`echo "scale=3; $tempoin + $tmp_tempoin" | bc`

    riga=${riga#*|}  #tempo in coda|code
    tmp_tempocoda=${riga%%|*} #tempo in coda
    tempocoda=`echo "scale=3; $tempocoda + $tmp_tempocoda" | bc`

    riga=${riga#*|} #code
    if [ $riga -ge 2 ]; then
        cambiocode=$(($cambiocode + $riga -1));
        clienti_cambiocode=$(($clienti_cambiocode + 1));
    fi

done

#CLIENTI USCITI FORZATAMENTE
cat ./log/* | grep -e "CQ|" | sort > ./log/tmp_clienti_quit
exec 4<./log/tmp_clienti_quit

while read -u 4 riga; do
    echo $riga
    clienti_quit=$(($clienti_quit + 1))
done

#CASSE
#K|id cassa|n. prodotti elaborati|n. di clienti|tempo tot. di apertura|tempo medio di servizio|n. di chiusure
#VARIABILI
casse=0
prodotti_elaborati=0
tot_prodotti_elaborati=0
num_clienti_serviti=0
tot_tempo_apertura=0.000
tot_tempo_servizio=0.000
tot_chiusure=0
K_chiusure=0
cat ./log/* | grep -e "K|" | sort > ./log/tmp_casse
exec 5<./log/tmp_casse
while read -u 5 riga; do
    echo $riga
    riga=${riga#K|*|}   #n. prodotti elaborati|n. di clienti|tempo tot. di apertura|tempo medio di servizio|n. di chiusure

    tmp_prodotti_elaborati=${riga%%|*} #n. prodotti elaborati
    riga=${riga#*|}  #n. di clienti|tempo tot. di apertura|tempo medio di servizio|n. di chiusure

    tmp_num_clienti_serviti=${riga%%|*}  #n. di clienti
    riga=${riga#*|} #tempo tot. di apertura|tempo medio di servizio|n. di chiusure

    tmp_tempo_apertura=${riga%%|*} #tempo tot. di apertura
    riga=${riga#*|} #tempo medio di servizio|n. di chiusure

    tmp_tempo_servizio=${riga%%|*} #tempo medio di servizio
    riga=${riga#*|} #n. di chiusure

    if (( $(echo "$tmp_tempo_apertura > 0.000" |bc -l) )); then
        casse=$(($casse + 1))
        tot_tempo_apertura=`echo "scale=3; $tot_tempo_apertura + $tmp_tempo_apertura" | bc`
        tot_tempo_servizio=`echo "scale=3; $tot_tempo_servizio + $tmp_tempo_servizio" | bc`
        num_clienti_serviti=$(($num_clienti_serviti + $tmp_num_clienti_serviti))
        tot_prodotti_elaborati=$(($tot_prodotti_elaborati + $tmp_prodotti_elaborati))
        if [ $riga -ne 0 ]; then
            tot_chiusure=$(($tot_chiusure + $riga))
            K_chiusure=$(( $K_chiusure + 1))
        fi
    fi
done

#CLIENTI
tot_clienti=$(($clienti + $clienti_quit))
echo "- In totale sono entrati ${tot_clienti} clienti"
if [ $clienti_quit -ne 0 ]; then
    echo "- ${clienti_quit} clienti sono stati fatti uscire forzatamente"
fi

if [ $clienti_noprod -ne 0 ]; then
    if [ $clienti_noprod == 1 ]; then
        echo "- ${clienti_noprod} cliente non ha acquistato prodotti";
    else
        echo "- ${clienti_noprod} clienti non hanno acquistato prodotti";
    fi
fi

#PRODOTTI ACQUISTATI
prodotti_media=`echo "scale=1; $prodotti / $tot_clienti" | bc`
echo "- Sono stati acquistati un totale di ${prodotti}, mediamente ${prodotti_media} a cliente"

#TEMPO CLIENTI
tempoin=${tempoin%.*}
tempocoda=${tempocoda%.*}
echo "- È stato speso un totale di ${tempoin} secondi all'interno del supermercato, di cui ${tempocoda} in coda"
tempoin_media=`echo "scale=3; $tempoin / $tot_clienti" | bc`
tempocoda_media=`echo "scale=3; $tempocoda / $tot_clienti" | bc`
echo "- Ogni cliente ha passato in media ${tempoin_media} secondi nel supermercato di cui ${tempocoda_media} secondi in coda"

#CODE
if [ $cambiocode -eq 0 ]; then
    echo "- Nessun cliente ha cambiato coda";
else
    if [ $cambiocode == 1 ]; then
        echo "- È stata cambiata coda una volta da un cliente";
    else
        if [ ${clienti_cambiocode} == 1 ]; then
            echo "- È stata cambiata coda ${cambiocode} volte da un cliente";
        else
            echo "- È stata cambiata coda ${cambiocode} volte da un totale di ${clienti_cambiocode} clienti diversi";
        fi
    fi
fi

#CASSE
num_clienti_serviti_medi=`echo "scale=1; $num_clienti_serviti / $casse" | bc`
echo "- Sono state aperte ${casse} casse che hanno servito ${num_clienti_serviti} clienti, mediamente ${num_clienti_serviti_medi} a cassa"

#PRODOTTI ELABORATI
prodotti_elaborati_medi=`echo "scale=1; $tot_prodotti_elaborati / $casse" | bc`
echo "- Sono stati elaborati ${tot_prodotti_elaborati} prodotti, mediamente ${prodotti_elaborati_medi} a cassa"

#TEMPO CASSE
tempo_apertura_medio=`echo "scale=3; $tot_tempo_apertura / $casse" | bc`
echo "- Le somma dell'apertura delle casse è ${tot_tempo_apertura} secondi, mediamente ${tempo_apertura_medio} a cassa"

tempo_servizio_medio=`echo "scale=3; $tot_tempo_servizio / $casse" | bc`
echo "- Il tempo di servizio medio è di ${tempo_servizio_medio} secondi"

#CHIUSURE
if [ $tot_chiusure -eq 0 ]; then
    echo "- Il direttore non ha mai chiuso le casse se non alla fine"
else
    if [ $K_chiusure -ne 1 ]; then
        echo "- In totale le chiusure imposte dal direttore sono state ${tot_chiusure} di ${K_chiusure} casse diverse"
    else
        echo "- In totale le chiusure imposte dal direttore sono state ${tot_chiusure} di ${K_chiusure} cassa"
    fi
fi

echo "### FINE ANALISI LOG ###"

#PULIZIA
rm ./log/tmp*
