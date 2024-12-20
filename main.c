#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Buat alias untuk membuat direktori untuk windows
// karena fungsinya berbeda dari fungsi linux
#ifdef _WIN32
#include <windows.h>
#define mkdir(dir, mode) CreateDirectory(dir, NULL)
#else
#include <sys/stat.h>
#endif

// Struct ini digunakan untuk mencatat tipe masukan/keluaran dari kedai
typedef struct
{
    int   id;
    char *nama;
    int   harga_dasar;     // Harga hanya digunakan untuk masukan
} JenisTransaksi;

// Struct ini memegang semua tipe masukan dan keluaran
typedef struct
{
    JenisTransaksi *pemasukan;
    int             jumlah_pemasukan;

    JenisTransaksi *pengeluaran;
    int             jumlah_pengeluaran;
} KoleksiJenis;

// Struct ini digunakan untuk membantu pendataan entri masukan/keluaran kedai
typedef struct
{
    int id;
    int frekuensi;     // Untuk pendapatan berupa total, bukan jumlah muncul saja
    int harga_kumulatif;
} EntriTransaksi;

// --- Fungsi Pembantu --- //

// Alokasikan string dinamis dari buffer
void alokasiBuffer(char **target, char *buffer)
{
    if (*target != NULL)
        free(*target);

    *target = calloc(strlen(buffer) + 1, sizeof(char));
    strncpy(*target, buffer, strlen(buffer));
}

// Bersihkan buffer input pengguna
void bersihkanInput()
{
    int k = getchar();
    while (k != '\n' && k != EOF)
        k = getchar();
}

// Masukkan angka dari pengguna
int masukkanAngka(int nilai_default, int wajib)
{
    char buffer[16]  = {0};
    int  hasil       = 0;
    int  perlu_ambil = 1;
    int  hasil_parse = 0;

    while (perlu_ambil)
    {
        fgets(buffer, sizeof(buffer), stdin);
        if (buffer[strlen(buffer) - 1] != '\n')
            bersihkanInput();

        if (buffer[0] == '\n' && !wajib)
            return nilai_default;

        hasil_parse = sscanf(buffer, "%d", &hasil);
        if (hasil_parse == 1)
            perlu_ambil = 0;
    }
    return hasil;
}

// Ambil pilihan y/t dari pengguna, nilai default digunakan jika
// masukan pengguna kosong
int masukkanKonfirmasi(int nilai_default)
{
    int hasil = nilai_default;
    int k     = getchar();

    if (k != '\n')
        bersihkanInput();

    if (toupper(k) == 'Y' || toupper(k) == 'T')
        hasil = toupper(k) == 'Y';

    return hasil;
}

// Ambil masukan dalam bentuk string dari pengguna dan kembalikan pointer
// ke string dinamis
void masukkanStringDinamis(char **tujuan, char *nilai_default)
{
    int  perlu_ambil = 1;
    char buffer[128] = {0};

    while (perlu_ambil)
    {
        fgets(buffer, sizeof(buffer), stdin);
        if (buffer[strlen(buffer) - 1] != '\n')
            bersihkanInput();

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0 && nilai_default != NULL)
        {
            strncpy(buffer, nilai_default, sizeof(buffer));
            perlu_ambil = 0;
        }
        else if (strlen(buffer) > 0)
            perlu_ambil = 0;
    }

    alokasiBuffer(tujuan, buffer);
}

// Fungsi untuk mencari jenis dengan id tertentu dari koleksi jenis transaksi
JenisTransaksi *cariJenis(JenisTransaksi *koleksi, int jumlah_koleksi, int id)
{
    // Cari koleksi jenis transaksi yang diberikan untuk elemen dengan
    // id yang sama, kembalikan NULL jika tidak ada ditemukan
    for (int i = 0; i < jumlah_koleksi; i++)
        if (koleksi[i].id == id)
            return &koleksi[i];

    return NULL;
}

// Membuka dan mengembalikan file catatan transaksi berdasarkan bulan dan tahun sekarang
FILE *bukaFileCatatan()
{
    FILE *hasil = NULL;

    time_t     w    = time(NULL);
    struct tm *skrg = localtime(&w);

    int thn_skrg = skrg->tm_year + 1900;
    int bln_skrg = skrg->tm_mon + 1;

    char lokasi_file[24] = {0};
    sprintf(lokasi_file, "catatan/%d-%d.txt", thn_skrg, bln_skrg);

    // Pastikan folder tujuan ada
    mkdir("catatan", 0755);

    hasil = fopen(lokasi_file, "a");

    return hasil;
}

// --- Fungsi Utama --- //

// Muat tipe-tipe pemasukan dan pengeluaran usaha dari file database
void muatDatabase(KoleksiJenis *koleksi)
{
    // Lokasi file relatif ke program adalah
    //  ./data/pemasukan.txt
    //  ./data/pengeluaran.txt
    FILE *file_pemasukan;
    FILE *file_pengeluaran;

    koleksi->jumlah_pemasukan   = 0;
    koleksi->jumlah_pengeluaran = 0;

    char buffer[128] = {0};

    file_pemasukan = fopen("data/pemasukan.txt", "r");
    if (file_pemasukan != NULL)
    {
        // Jumlah macam disimpan pada awal file, baca sampai akhir setelah itu
        fscanf(file_pemasukan, "%d\n", &koleksi->jumlah_pemasukan);
        if (koleksi->jumlah_pemasukan > 0)
        {
            koleksi->pemasukan = malloc(koleksi->jumlah_pemasukan * sizeof(JenisTransaksi));
            for (int i = 0; i < koleksi->jumlah_pemasukan; i++)
            {
                fscanf(file_pemasukan, "%d;;%d;;%127[^\n]\n",
                       &koleksi->pemasukan[i].id,
                       &koleksi->pemasukan[i].harga_dasar,
                       buffer);
                alokasiBuffer(&koleksi->pemasukan[i].nama, buffer);
            }
        }
        else
            koleksi->pemasukan = NULL;

        fclose(file_pemasukan);
    }

    file_pengeluaran = fopen("data/pengeluaran.txt", "r");
    if (file_pengeluaran != NULL)
    {
        // Jumlah macam disimpan pada awal file, baca sampai akhir setelah itu
        fscanf(file_pengeluaran, "%d\n", &koleksi->jumlah_pengeluaran);
        if (koleksi->jumlah_pengeluaran > 0)
        {
            koleksi->pengeluaran = malloc(koleksi->jumlah_pengeluaran * sizeof(JenisTransaksi));
            for (int i = 0; i < koleksi->jumlah_pengeluaran; i++)
            {
                fscanf(file_pengeluaran, "%d;;%127[^\n]\n",
                       &koleksi->pengeluaran[i].id,
                       buffer);
                alokasiBuffer(&koleksi->pengeluaran[i].nama, buffer);
            }
        }
        else
            koleksi->pengeluaran = NULL;

        fclose(file_pengeluaran);
    }
}

// Simpan tipe-tipe pemasukan dan pengeluaran usaha dari file database
void simpanDatabase(KoleksiJenis koleksi)
{
    // Lokasi file relatif ke program adalah
    //  ./data/pemasukan.txt
    //  ./data/pengeluaran.txt
    FILE *file_pemasukan;
    FILE *file_pengeluaran;

    // Pastikan folder data ada
    mkdir("data", 0755);

    file_pemasukan = fopen("data/pemasukan.txt", "w");
    if (file_pemasukan != NULL)
    {
        // Jumlah macam disimpan pada awal file
        fprintf(file_pemasukan, "%d\n", koleksi.jumlah_pemasukan);
        for (int i = 0; i < koleksi.jumlah_pemasukan; i++)
        {
            // Simpan data
            // gunakan ;; sebagai pembatas
            fprintf(file_pemasukan, "%d;;%d;;%s\n",
                    koleksi.pemasukan[i].id,
                    koleksi.pemasukan[i].harga_dasar,
                    koleksi.pemasukan[i].nama);
        }

        fclose(file_pemasukan);
    }

    file_pengeluaran = fopen("data/pengeluaran.txt", "w");
    if (file_pengeluaran != NULL)
    {
        // Jumlah macam disimpan pada awal file
        fprintf(file_pengeluaran, "%d\n", koleksi.jumlah_pengeluaran);
        for (int i = 0; i < koleksi.jumlah_pengeluaran; i++)
        {
            // Simpan data
            // gunakan ;; sebagai pembatas
            fprintf(file_pengeluaran, "%d;;%s\n",
                    koleksi.pengeluaran[i].id,
                    koleksi.pengeluaran[i].nama);
        }

        fclose(file_pengeluaran);
    }
}

// Menu mencatat pemasukan baru
void catatMasukan(KoleksiJenis koleksi)
{
    int id_produk     = 0;
    int masukkan_lagi = 0;

    int harga_produk  = 0;
    int jumlah_produk = 0;
    int diskon        = 0;

    JenisTransaksi *templat_transaksi;

    do
    {
        printf("--- Catat Pemasukan ---\n");
        printf("Masukkan ID produk\n");
        printf("[-1] Batal\n");
        printf("ID: ");
        id_produk = masukkanAngka(-1, 0);

        if (id_produk == -1)
        {
            printf("Batal catat\n");
            return;
        }

        templat_transaksi = cariJenis(koleksi.pemasukan, koleksi.jumlah_pemasukan, id_produk);
        if (templat_transaksi != NULL)
        {
            printf("Masukkan harga satuan produk (kosongkan untuk %d): ",
                   templat_transaksi->harga_dasar);
            harga_produk = masukkanAngka(templat_transaksi->harga_dasar, 0);
            printf("Masukkan jumlah produk dalam transaksi (kosongkan untuk 1): ");
            jumlah_produk = masukkanAngka(1, 0);
            printf("Masukkan diskon (kosongkan untuk 0): ");
            diskon = masukkanAngka(0, 0);

            FILE *file_cttn = bukaFileCatatan();
            // + (pemasukan), id, harga, jumlah, diskon
            fprintf(file_cttn, "+;;%d;;%d;;%d;;%d\n", id_produk, harga_produk, jumlah_produk, diskon);
            fclose(file_cttn);
        }
        else
        {
            printf("Produk dengan ID %d tidak ditemukan\n", id_produk);
        }

        printf("Berhasil mencatat pemasukan\n");
        printf("Masukkan lagi (y/T): ");
        masukkan_lagi = masukkanKonfirmasi(0);

    } while (masukkan_lagi);
}

// Menu mencatat pengeluaran baru
void catatKeluaran(KoleksiJenis koleksi)
{
    int   id_pengeluaran     = 0;
    int   jumlah_pengeluaran = 0;
    char *nama_pengeluaran   = NULL;

    JenisTransaksi *templat_transaksi;

    printf("--- Catat Pengeluaran ---\n");
    printf("Masukkan tipe Pengeluaran\n");
    printf("[ 0] Lainnya\n");
    for (int i = 0; i < koleksi.jumlah_pengeluaran; i++)
    {
        printf("[ %d] %s\n", i + 1, koleksi.pengeluaran[i].nama);
    }

    printf("[-1] Batal\n");
    printf("ID: ");
    id_pengeluaran = masukkanAngka(-1, 0);

    if (id_pengeluaran == -1)
    {
        printf("Batal catat\n");
        return;
    }

    templat_transaksi = cariJenis(koleksi.pengeluaran, koleksi.jumlah_pengeluaran, id_pengeluaran);
    if (templat_transaksi != NULL || id_pengeluaran == 0)
    {
        if (id_pengeluaran == 0)
            masukkanStringDinamis(&nama_pengeluaran, NULL);
        else
            alokasiBuffer(&nama_pengeluaran, templat_transaksi->nama);

        printf("Masukkan jumlah pengeluaran: ");
        jumlah_pengeluaran = masukkanAngka(0, 1);

        FILE *file_pengeluaran = bukaFileCatatan();
        // - (pengeluaran), id, harga, nama (hanya jika custom)
        fprintf(file_pengeluaran, "-;;%d;;%d", id_pengeluaran, jumlah_pengeluaran);
        if (id_pengeluaran == 0)
            fprintf(file_pengeluaran, ";;%s", nama_pengeluaran);

        fprintf(file_pengeluaran, "\n");

        printf("Berhasil mencatat pengeluaran\n");

        fclose(file_pengeluaran);
        free(nama_pengeluaran);
    }
    else
    {
        printf("Pengeluaran tipe %d tidak ditemukan\n", id_pengeluaran);
    }
}

// Hitung dan tampilkan data tentang: total pemasukan, total pengeluaran,
// total pendapatan, pemasukan terbanyak berdasarkan jumlah penjualan,
// dan pengeluaran terurut dari terbanyak ke tersedikit
void lihatSimpulan(KoleksiJenis koleksi)
{
    int  bulan           = 0;
    int  tahun           = 0;
    int  jumlah_parse    = 0;
    char buffer[64]      = {0};
    char lokasi_file[64] = {0};

    EntriTransaksi *koleksi_pemasukan   = NULL;
    EntriTransaksi *koleksi_pengeluaran = NULL;

    char tipe_transaksi;

    int total_pemasukan   = 0;
    int total_pengeluaran = 0;

    FILE *file_catatan = NULL;

    printf("--- Rangkuman Pendapatan ---\n");
    printf("Masukkan bulan yang ingin dicek (bulan/tahun): ");

    // Verifikasi bahwa pengguna memasukkan tanggal dengan benar
    while (1)
    {
        fgets(buffer, sizeof(buffer), stdin);
        if (buffer[strlen(buffer) - 1] != '\n')
            bersihkanInput();

        buffer[strcspn(buffer, "\n")] = '\0';

        jumlah_parse = sscanf(buffer, "%d/%d", &bulan, &tahun);
        if (jumlah_parse == 2)
            break;
    }

    // format: "catatan/%d-%d.txt" (tahun-bulan)
    sprintf(lokasi_file, "catatan/%d-%d.txt", tahun, bulan);
    file_catatan = fopen(lokasi_file, "r");

    if (file_catatan == NULL)
    {
        printf("Tidak ada catatan untuk bulan %d/%d\n", bulan, tahun);
        return;
    }

    // Susun penghitung dengan tipe-tipe yang ada
    koleksi_pemasukan   = calloc(koleksi.jumlah_pemasukan, sizeof(EntriTransaksi));
    koleksi_pengeluaran = calloc(koleksi.jumlah_pengeluaran, sizeof(EntriTransaksi));

    for (int i = 0; i < koleksi.jumlah_pemasukan; i++)
        koleksi_pemasukan[i].id = koleksi.pemasukan[i].id;

    for (int i = 0; i < koleksi.jumlah_pengeluaran; i++)
        koleksi_pengeluaran[i].id = koleksi.pengeluaran[i].id;

    // Baca file sampai akhir dan hitung frekuensi dan harga
    // kumulatif tiap jenis barang/pengeluaran
    printf("Membaca file...\n");
    while (fscanf(file_catatan, "%c;;", &tipe_transaksi) != EOF)
    {
        EntriTransaksi *transaksi = NULL;

        int id            = 0;
        int harga_dasar   = 0;
        int jumlah_barang = 0;
        int diskon        = 0;

        // + (pemasukan), id, harga, jumlah, diskon
        if (tipe_transaksi == '+')
        {
            fscanf(file_catatan, "%d;;%d;;%d;;%d\n", &id, &harga_dasar, &jumlah_barang, &diskon);
            for (int i = 0; i < koleksi.jumlah_pemasukan; i++)
                if (koleksi_pemasukan[i].id == id)
                {
                    transaksi = &koleksi_pemasukan[i];
                    break;
                }

            transaksi->frekuensi += jumlah_barang;
            transaksi->harga_kumulatif += harga_dasar * jumlah_barang - diskon;
        }
        if (tipe_transaksi == '-')
        {
            // - (pengeluaran), id, harga, nama (hanya jika custom)
            fscanf(file_catatan, "%d;;%d", &id, &harga_dasar);
            if (id == 0)
                fscanf(file_catatan, ";;%*[^\n]");
            fscanf(file_catatan, "\n");

            for (int i = 0; i < koleksi.jumlah_pengeluaran; i++)
                if (koleksi_pengeluaran[i].id == id)
                {
                    transaksi = &koleksi_pengeluaran[i];
                    break;
                }

            transaksi->frekuensi += 1;
            transaksi->harga_kumulatif += harga_dasar;
        }
    }

    // Hitung total pemasukan dan pengeluaran dalam bulan
    printf("Menghitung total...\n");
    for (int i = 0; i < koleksi.jumlah_pemasukan; i++)
        total_pemasukan += koleksi_pemasukan[i].harga_kumulatif;

    for (int i = 0; i < koleksi.jumlah_pengeluaran; i++)
        total_pengeluaran += koleksi_pengeluaran[i].harga_kumulatif;

    printf("Mengurutkan...\n");
    {
        // Urutkan pemasukan berdasarkan frekuensi dari tertinggi ke terrendah
        // Gunakan "selection sort"

        int frek_max   = 0;
        int indeks_max = 0;

        EntriTransaksi smntr;
        for (int i = 0; i < koleksi.jumlah_pemasukan; i++)
        {
            frek_max   = 0;
            indeks_max = i;
            for (int j = i; j < koleksi.jumlah_pemasukan; j++)
            {
                if (koleksi_pemasukan[j].frekuensi > frek_max)
                {
                    frek_max   = koleksi_pemasukan[i].frekuensi;
                    indeks_max = j;
                }
            }
            smntr                         = koleksi_pemasukan[i];
            koleksi_pemasukan[i]          = koleksi_pemasukan[indeks_max];
            koleksi_pemasukan[indeks_max] = smntr;
        }
    }

    {
        // Urutkan pengeluaran berdasarkan jumlah pengeluaran dari tertinggi ke terrendah
        // Gunakan "selection sort"

        int max      = 0;
        int idks_max = 0;

        EntriTransaksi smntr;
        for (int i = 0; i < koleksi.jumlah_pengeluaran; i++)
        {
            max      = 0;
            idks_max = i;
            for (int j = i; j < koleksi.jumlah_pengeluaran; j++)
            {
                if (koleksi_pengeluaran[j].harga_kumulatif > max)
                {
                    max      = koleksi_pengeluaran[i].harga_kumulatif;
                    idks_max = j;
                }
            }
            smntr                         = koleksi_pengeluaran[i];
            koleksi_pengeluaran[i]        = koleksi_pengeluaran[idks_max];
            koleksi_pengeluaran[idks_max] = smntr;
        }
    }

    JenisTransaksi *data_transaksi = NULL;

    printf("--- Hasil Rangkuman ---\n");
    printf("Total Masukan    : %d\n", total_pemasukan);
    printf("Total Pengeluaran: %d\n", total_pengeluaran);
    printf("Total Pendapatan : %d\n", total_pemasukan - total_pengeluaran);
    printf("Pendapatan Terurut Berdasarkan Frekuensi Pembelian:\n");
    printf("ID    Frekuensi      Nama\n");
    for (int i = 0; i < koleksi.jumlah_pemasukan; i++)
    {
        data_transaksi = cariJenis(koleksi.pemasukan, koleksi.jumlah_pemasukan, koleksi_pemasukan[i].id);
        printf("%-5d %-14d %-s\n",
               koleksi_pemasukan[i].id,
               koleksi_pemasukan[i].frekuensi,
               data_transaksi->nama);
    }
    printf("Pendapatan Terurut Berdasarkan Jumlah:\n");

    {
        // Urutkan pemasukan berdasarkan jumlah dari tertinggi ke terrendah
        // Gunakan "selection sort"

        int maks      = 0;
        int idks_maks = 0;

        EntriTransaksi smntr;
        for (int i = 0; i < koleksi.jumlah_pemasukan; i++)
        {
            maks      = 0;
            idks_maks = i;
            for (int j = i; j < koleksi.jumlah_pemasukan; j++)
            {
                if (koleksi_pemasukan[j].harga_kumulatif > maks)
                {
                    maks      = koleksi_pemasukan[i].harga_kumulatif;
                    idks_maks = j;
                }
            }
            smntr                        = koleksi_pemasukan[i];
            koleksi_pemasukan[i]         = koleksi_pemasukan[idks_maks];
            koleksi_pemasukan[idks_maks] = smntr;
        }
    }

    printf("ID    Jumlah         Nama\n");
    for (int i = 0; i < koleksi.jumlah_pemasukan; i++)
    {
        data_transaksi = cariJenis(
            koleksi.pemasukan,
            koleksi.jumlah_pemasukan,
            koleksi_pemasukan[i].id);
        printf("%-5d %-14d %-s\n",
               koleksi_pemasukan[i].id,
               koleksi_pemasukan[i].harga_kumulatif,
               data_transaksi->nama);
    }

    printf("Pengeluaran Terurut Berdasarkan Jumlah:\n");
    printf("ID    Jumlah         Nama\n");
    for (int i = 0; i < koleksi.jumlah_pengeluaran; i++)
    {
        data_transaksi = cariJenis(
            koleksi.pengeluaran,
            koleksi.jumlah_pengeluaran,
            koleksi_pengeluaran[i].id);
        printf("%-5d %-14d %-s\n",
               koleksi_pengeluaran[i].id,
               koleksi_pengeluaran[i].harga_kumulatif,
               data_transaksi->nama);
    }
    printf("-----------------------\n");

    free(koleksi_pemasukan);
    free(koleksi_pengeluaran);
    fclose(file_catatan);
}

// Tambah/Edit katalog produk dan jenis-jenis pengeluaran
void editDatabase(KoleksiJenis *koleksi)
{
    int             pilihan;
    int             id;
    int             elemen_baru   = 0;
    JenisTransaksi *elemen_tujuan = NULL;
    JenisTransaksi *smtr          = NULL;

    printf("--- Edit database jenis pemasukan/pengeluaran ---\n");
    printf("[1] Edit pemasukan\n");
    printf("[2] Edit pengeluaran\n");
    printf("[-1] Batal\n");
    printf("Pilih jenis yang ingin diedit: ");
    pilihan = masukkanAngka(0, 1);

    switch (pilihan)
    {
        case 1:
            printf("[-1] Batal\n");
            printf("Masukkan ID Produk: ");
            id = masukkanAngka(0, 1);

            if (id == -1)
            {
                printf("Edit dibatalkan\n");
                return;
            }

            elemen_tujuan = cariJenis(koleksi->pemasukan, koleksi->jumlah_pemasukan, id);

            if (elemen_tujuan == NULL)
            {
                smtr = realloc(
                    koleksi->pemasukan,
                    sizeof(JenisTransaksi) * (koleksi->jumlah_pemasukan + 1));
                if (smtr == NULL)
                {
                    printf("ERROR: Alokasi gagal\n");
                    return;
                }

                koleksi->pemasukan = smtr;
                koleksi->jumlah_pemasukan += 1;

                elemen_tujuan = &koleksi->pemasukan[koleksi->jumlah_pemasukan - 1];

                elemen_tujuan->id = id;

                elemen_tujuan->harga_dasar = 0;
                elemen_tujuan->nama        = NULL;
            }

            // Minta pengguna untuk memasukkan nilai baru nama dan harga produk,
            elemen_baru = elemen_tujuan->nama == NULL;

            // Jika bukan produk baru maka tunjukkan juga nilai sebelumnya
            printf("Masukkan nama baru produk");
            if (!elemen_baru)
                printf(" (%s)", elemen_tujuan->nama);
            printf(": ");

            masukkanStringDinamis(&elemen_tujuan->nama, elemen_tujuan->nama);

            printf("Masukkan harga satuan baru produk");
            if (!elemen_baru)
                printf(" (%d)", elemen_tujuan->harga_dasar);
            printf(": ");

            // Untuk elemen baru, harga produk wajib dimasukkan
            elemen_tujuan->harga_dasar = masukkanAngka(elemen_tujuan->harga_dasar, elemen_baru);

            printf("Berhasil mengedit entri\n");
            break;
        case 2:
            printf("[-1] Batal\n");
            printf("Masukkan ID Tipe Pengeluaran: ");
            id = masukkanAngka(0, 1);

            if (id == 0)
            {
                printf("Pengeluaran tipe \"Lainnya\" tidak dapat diedit\n");
                return;
            }
            else if (id == -1)
            {
                printf("Edit dibatalkan\n");
                return;
            }

            // Cari pengeluaran dengan id tipe yang sama
            elemen_tujuan = cariJenis(koleksi->pengeluaran, koleksi->jumlah_pengeluaran, id);

            // Jika tidak ada, buat tipe baru
            if (elemen_tujuan == NULL)
            {
                smtr = realloc(
                    koleksi->pengeluaran,
                    sizeof(JenisTransaksi) * (koleksi->jumlah_pengeluaran + 1));
                if (smtr == NULL)
                {
                    printf("ERROR: Alokasi gagal\n");
                    return;
                }

                koleksi->pengeluaran = smtr;
                koleksi->jumlah_pengeluaran += 1;

                elemen_tujuan = &koleksi->pengeluaran[koleksi->jumlah_pengeluaran - 1];

                elemen_tujuan->id   = id;
                elemen_tujuan->nama = NULL;
            }

            // Minta pengguna untuk memasukkan nilai baru nama tipe pengeluaran
            elemen_baru = elemen_tujuan->nama == NULL;

            // Jika bukan pengeluaran baru maka tunjukkan juga nilai sebelumnya
            printf("Masukkan nama baru tipe pengeluaran");
            if (!elemen_baru)
                printf(" (%s)", elemen_tujuan->nama);
            printf(": ");

            masukkanStringDinamis(&elemen_tujuan->nama, elemen_tujuan->nama);

            printf("Berhasil mengedit entri\n");
            break;
        case -1:
            printf("Edit dibatalkan\n");
            break;
        default:
            printf("Pilihan tidak valid.\n");
            break;
    }
}

int main()
{
    // Array jenis-jenis masukan dan keluaran
    KoleksiJenis koleksi_jenis;

    // Muat database jenis-jenis masukan dan keluaran
    muatDatabase(&koleksi_jenis);

    int berhenti = 0;
    int pilihan  = 0;

    while (berhenti == 0)
    {
        printf("Pembukuan Digital Kedai\n");
        printf("\n");

        printf("--- Menu Utama ---\n");
        printf("[1] Catat Masukkan\n");
        printf("[2] Catat Keluaran\n");
        printf("[3] Lihat Simpulan\n");
        printf("[4] Edit Database\n");
        printf("[5] Keluar program\n");
        printf("------------------\n");
        printf("Pilih aksi: ");

        pilihan = masukkanAngka(-1, 1);

        switch (pilihan)
        {
            case 1:
                catatMasukan(koleksi_jenis);
                break;
            case 2:
                catatKeluaran(koleksi_jenis);
                break;
            case 3:
                lihatSimpulan(koleksi_jenis);
                break;
            case 4:
                editDatabase(&koleksi_jenis);
                break;
            case 5:
                printf("Keluar dari program\n");
                berhenti = 1;
                break;
            default:
                printf("Pilihan tidak valid\n");
                break;
        }
    }

    simpanDatabase(koleksi_jenis);

    return 0;
}