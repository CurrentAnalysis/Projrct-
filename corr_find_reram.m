clear
import mlreportgen.dom.*;
%%
% Reading data %
Array_Reram=csvread('RERAM_HelloWorld.csv',1);
x=linspace(0,500,length(Array_Reram));
figure;
plot(x,Array_Reram);

%%
%smaple the postive values
x_pos = x(find(Array_Reram>0)');
Array_Reram_postive = Array_Reram(find(Array_Reram > 0)');

%%
%find signal peaks
[Array_Reram_pks,locs] = findpeaks(Array_Reram_postive,'MinPeakDistance',100);
mean_peaks = mean(Array_Reram_pks);
figure;
plot(x_pos,Array_Reram_postive,x_pos(locs), Array_Reram_pks,'or');

%%
%filter noise peaks out of clocked data signal 
Array_Reram_pks(find(Array_Reram_pks < mean_peaks)')= 0;
Array_Reram_pks_filter = Array_Reram_pks(find(Array_Reram_pks > 0)');

locs = locs(find(Array_Reram_pks>0)');

locs = locs(1:end-2);
Array_Reram_pks_filter= Array_Reram_pks_filter(1:end-2);
figure;
plot(x_pos,Array_Reram_postive,x_pos(locs), Array_Reram_pks_filter,'or');

%%
%find cut
target_peaks = zeros(16,12);
for n = 1 : 12
    target_peaks(:,n) = Array_Reram_pks_filter(end - (n-1)*16 -15 :end-(n-1)*16);
    figure;
    plot(x_pos,Array_Reram_postive,x_pos(locs(end - (n-1)*16 -15 :end-(n-1)*16)),...
        Array_Reram_pks_filter(end - (n-1)*16 -15 :end-(n-1)*16),'or');
end

%%
%load data base
Array_database_peaks=csvread('array_database_peaks_Reram_1.csv');
corr_array_results= zeros(2650,1);
tmp_pattren_peaks= zeros(16,1);
target_hypo_data = strings(12,1);

%%  find max corr between target signal and database peaks
%
for target_index = 1 : 12
    % find corr between target signal and database peaks
    for n = 1 : 2560
        tmp_pattren_peaks = Array_database_peaks(:,n);
        rho = corr(target_peaks(1:end-1,target_index),tmp_pattren_peaks(1:end-1));
        corr_array_results(n) = rho;
    end

    %find hypothesis traget string by finding max corr
    [max_values,max_indexs] = sort(corr_array_results','descend');
    hypo = max_indexs(1:10);
end   
    
    decode_near = max_indexs(11:20);
    decode_far = max_indexs(end-9:end);
    for n = 1 : 12
        if(mod(hypo(n),10) == 0)
            hypo(n) = hypo(n) -1;
        end
    end
    
    %plot hypo string
    char_hypo = char(hypo/10);
    char_hypo(1:10)
%     char_decode_far = mode(char(decode_far/10))
     char_decode_near = mode(char(decode_near/10));

    target_hypo_data(target_index) = mode(char_hypo);
end
%% plot results
target_hypo_data = flipud(target_hypo_data)

figure;
subplot(1,1,1)
plot(x_pos,Array_Reram_postive,x_pos(locs), Array_Reram_pks_filter,'or');
title('ReRam - Power Supply Current Measurement - Hello World');
ylabel('uA');
xlabel('usec');

hold on;
[X,Y] = meshgrid(x_pos,mean_peaks);
plot(X,Y);
