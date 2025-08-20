<div align="center">
  <h1 align="center">Progetto sistemi operativi: Pthread_SHA256</h1>

  <p align="center">
    Progetto di un servizio di calcolo della impronta SHA-256 di file multipli.
    <br />
    <i>Project for a service to calculate the SHA-256 fingerprint of multiple files.</i>
    <br />
    <br />
    <a href="https://github.com/lorenzogasparini/ProgettoIngegneriaSw/tree/main/doc"><strong>Explore the docs »</strong></a>
    <br />
    <a href="https://github.com/lorenzogasparini/ProgettoIngegneriaSw/issues/new?labels=bug&template=bug-report---.md">Report Bug</a>
    &middot;
    <a href="https://github.com/lorenzogasparini/ProgettoIngegneriaSw/issues/new?labels=enhancement&template=feature-request---.md">Request Feature</a>
  </p>
</div>

<!-- TABLE OF CONTENTS
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a></li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <li><a href="#acknowledgments">Acknowledgments</a></li>
  </ol>
</details>
-->

<!-- ABOUT THE PROJECT -->
## About The Project

SHA sta per Secure Hash Algorithm ed appartiene ad una famiglia di algoritmi che convertono una stringa di byte di lunghezza arbitraria in una impronta (digest in inglese) di lunghezza fissata in bit. Nel nostro caso si tratta di una impronta di 256 bit, ovvero 32 byte rappresentabili visivamente come una stringa di 64 simboli esadecimali. 

Viene utilizzata per sintetizzare contenuti informativi complessi in un formato compatto: è estremamente improbabile che due input diversi abbiano la stessa impronta. Usi classici sono per indicizzazione di dati tramite hashing oppure per verifica di integrità, per esempio controllando che l’impronta di un file scaricabile pubblicata ufficialmente combaci con quella computata sulla copia scaricata del file.

L’obiettivo è anzitutto realizzare un server che permetta multiple computazioni di impronte SHA-256. Il tempo di calcolo per impronta è proporzionale al numero di byte dell’ingresso (ovvero il file), dipendente
dalla piattaforma e dalla implementazione dell’algoritmo (∼1 secondo per 70 MB su laptop moderno nella implementazione OpenSSL, ∼1 secondo per 30 KB su MentOS in QEMU). Successivamente va realizzato un client che invii l’informazione di file di input al server e riceva l’impronta risultante appena computata.

L’obiettivo è anzitutto realizzare un server che permetta multiple computazioni di impronte SHA-256. Il tempo di calcolo per impronta è proporzionale al numero di byte dell’ingresso (ovvero il file), dipendente dalla piattaforma e dalla implementazione dell’algoritmo (∼1 secondo per 70 MB su laptop moderno nella implementazione OpenSSL, ∼1 secondo per 30 KB su MentOS in QEMU). Successivamente va realizzato un client che invii l’informazione di file di input al server e riceva l’impronta risultante appena computata.

<p align="center">
  <img src="https://github.com/lorenzogasparini/Pthread_SHA256Project/bin/test.gif" alt="Gif di test" />
</p>

### Built With

Di seguito si indicano le principali librerie, nonchè il sistema di database utilizzato per lo sviluppo del progetto.

* ![JavaFX](https://img.shields.io/badge/CMAKE-red?style=for-the-badge)

### Installation

Per compilare con il CMakelist:
1. Setup dei prerequisiti, elencati nella sezione Build With
2. ```sh
   mkdir build
   ```
3. ```sh
   cd build
   ```
4. ```sh
   cmake ..
   ```
5. ```sh
    make
   ```

[java-shield]: https://img.shields.io/badge/CMAKE-red?style=for-the-badge
[sqlite-shield]: https://img.shields.io/badge/Database-SQLite-blue?style=for-the-badge
