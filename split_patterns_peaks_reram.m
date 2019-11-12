
function [pattren_peaks] = split_patterns_peaks_reram(snapshot)

%postive

Array_pattren_postive = snapshot(find(snapshot > 0)');
x_pos = linspace(0,50,length(Array_pattren_postive));

%peaks
[Array_pattren_pks,pattren_locs] = findpeaks(Array_pattren_postive,'MinPeakDistance',150);
pattren_mean_peaks = mean(Array_pattren_pks);

%peaks filter
Array_pattren_pks(find(Array_pattren_pks < 0.3*10^-3)')= 0;
Array_pattren_pks_filter = Array_pattren_pks(find(Array_pattren_pks > 0)');
pattren_locs = pattren_locs(find(Array_pattren_pks>0)');
%cut
pattren_peaks = Array_pattren_pks_filter(end-15:end);
pattren_locs = pattren_locs(end-15:end);
% 
% 
figure;
plot(x_pos,Array_pattren_postive,x_pos(pattren_locs), pattren_peaks,'or');
end