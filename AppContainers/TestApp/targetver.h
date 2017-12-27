#pragma once

// Le macro seguenti definiscono la piattaforma minima richiesta. La piattaforma minima richiesta
// � costituita dalla versione meno recente di Windows, Internet Explorer e cos� via contenenti le funzionalit� necessarie per eseguire 
// l'applicazione. Le macro consentono di attivare tutte le funzionalit� disponibili nelle versioni delle piattaforme fino 
// alla versione specificata compresa.

// Modificare le seguenti definizioni se � necessario utilizzare come destinazione una piattaforma prima di quelle specificate di seguito.
// Fare riferimento a MSDN per informazioni aggiornate sui valori corrispondenti per le differenti piattaforme.
#ifndef WINVER                          // Specifica che la piattaforma minima richiesta � Windows Vista.
#define WINVER 0x0600           // Modificare il valore con quello appropriato per altre versioni di Windows.
#endif

#ifndef _WIN32_WINNT            // Specifica che la piattaforma minima richiesta � Windows Vista.
#define _WIN32_WINNT 0x0600     // Modificare il valore con quello appropriato per altre versioni di Windows.
#endif

#ifndef _WIN32_WINDOWS          // Specifica che la piattaforma minima richiesta � Windows 98.
#define _WIN32_WINDOWS 0x0410 // Modificare il valore con quello appropriato per Windows Me o versioni successive.
#endif

#ifndef _WIN32_IE                       // Specifica che la piattaforma minima richiesta � Internet Explorer 7.0.
#define _WIN32_IE 0x0700        // Modificare il valore con quello appropriato per altre versioni di IE.
#endif
