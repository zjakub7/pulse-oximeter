pkg load io
pkg load signal
clear; clc;

% Wczytanie danych z pliku
[data, ~, ~] = xlsread('pomiar_babcia_3.xlsx');
czas = data(:, 1);
czujnik1 = data(:, 2);
czujnik2 = data(:, 3);

% Parametry filtru 
fs = 500;
f_pass = [0.8 , 1.75];
transition = 0.25;
nyq = fs / 2;
fp = f_pass / nyq;

numtaps = 4001;
taps = fir1(numtaps - 1, [fp(1), fp(2)], 'bandpass', hamming(numtaps));  % OK

% Filtracja sygnałów
czujnik1_filtr = filter(taps, 1 ,czujnik1);
czujnik2_filtr = filter(taps, 1 , czujnik2);

%zakres probek do wyswietlenia
czujnik1_cut=(czujnik1_filtr(4000:9000));
czujnik2_cut=(czujnik2_filtr(4000:9000));
czas_cut=(czas(4000:9000));



% Wykresy
figure;
plot(czas_cut,czujnik1_cut,'b',czas_cut,czujnik2_cut,'r');%x-czas y-czujnik
title(sprintf('Czujnik 1 i Czujnik 2 po filtracji (%.1f–%.1f Hz)', f_pass(1), f_pass(2)));%tytul sprintf dla wyswietlenia wartosci
xlabel('Czas [s]');
ylabel('Amplituda');
legend('Czujnik 1', 'Czujnik 2');
grid on;

%wykres surowicy
%figure;
%plot(czas, czujnik1, 'b', czas, czujnik2,'r');
%title('Dane przed filtracją');
%xlabel('Czas [s]');
%ylabel('Amplituda');
%legend('Czujnik 1', 'Czujnik 2');
%grid on;

%korelacjaaa przez 1 s
%liczba probek 10-11s pomiaru
czujnik1_kor=(czujnik1_filtr(4829:5363));
czujnik2_kor=(czujnik2_filtr(4829:5363));
czas_kor=(czas(4829:5363));
% Korelacja wzajemna
[xc, lags] = xcorr(czujnik1_kor, czujnik2_kor, 'coeff');

% Maksimum korelacji – przesunięcie
%jezeli przesuniecie jest na + to czujnik 1 zarejestrowal dane szybcije
[~, idx_max] = max(xc);
lag_samples = lags(idx_max);           % przesunięcie w próbkach
lag_seconds = lag_samples / fs;        % przesunięcie w sekundach

% Wyświetl wynik
fprintf('Maksymalna korelacja przy przesunięciu: %d próbek (%.4f s)\n', lag_samples, lag_seconds);

% Wykres korelacji
figure;
plot(lags/fs, xc);
xlabel('Przesunięcie [s]');
ylabel('Współczynnik korelacji');
title('Korelacja wzajemna Czujnik1 vs Czujnik2');
grid on;
