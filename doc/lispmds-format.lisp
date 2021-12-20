;; MDS configuration file (version 0.6).
;; Created for TAB1 at 09:05:22 on 03/21/2016


(MAKE-MASTER-MDS-WINDOW
   (HI-IN '([antigen-name] ...)
          '([serum-name] ...)
          '(([titer: float or <float or *] ... *number-of-sera) ... *number-of-antigens)
          '[table-name]
          )
  :STARTING-COORDSS '(([float] ... *num-dimensions) ... *num-points  ; no closing )!
                      ((COL-AND-ROW-ADJUSTS
                        (-1.0d+7 ... *num-antigens
                         [column-basis: float] ... *num-sera
                         [avidity-adjust} ... *num-points))))
  :BATCH-RUNS '(((([float] ... *num-dimensions) ... *num-points  ; no closing )!
                  ((COL-AND-ROW-ADJUSTS
                    (-1.0d+7 ... *num-antigens
                     [column-basis: float] ... *num-sera
                     [avidity-adjust} ... *num-points))))
                 [stress:float] MULTIPLE-END-CONDITIONS NIL) ... *num-projections
                )
  :MDS-DIMENSIONS '2
  :MOVEABLE-COORDS 'ALL
  :UNMOVEABLE-COORDS 'NIL
  :CANVAS-COORD-TRANSFORMATIONS '(
                                  :CANVAS-WIDTH 530
                                                :CANVAS-HEIGHT 450
                                                :CANVAS-X-COORD-TRANSLATION 284.0
                                                :CANVAS-Y-COORD-TRANSLATION -4.0
                                                :CANVAS-X-COORD-SCALE 32.94357699173962d0
                                                :CANVAS-Y-COORD-SCALE 32.94357699173962d0
                                                :CANVAS-BASIS-VECTOR-0 (0.48512267784748486d0 -0.8744461032208247d0)
                                                :CANVAS-BASIS-VECTOR-1 (0.8744461032208257d0 0.48512267784748314d0)
                                                :FIRST-DIMENSION 0
                                                :SECOND-DIMENSION 1
                                                :BASIS-VECTOR-POINT-INDICES (0 1 2)
                                                :BASIS-VECTOR-POINT-INDICES-BACKUP NIL
                                                :BASIS-VECTOR-X-COORD-TRANSLATION 0
                                                :BASIS-VECTOR-Y-COORD-TRANSLATION 0
                                                :BASIS-VECTOR-X-COORD-SCALE 1
                                                :BASIS-VECTOR-Y-COORD-SCALE 1
                                                :TRANSLATE-TO-FIT-MDS-WINDOW T
                                                :SCALE-TO-FIT-MDS-WINDOW T)
  :CONSTANT-STRESS-RADIAL-DATA 'NIL
  :REFERENCE-ANTIGENS 'NIL
  :PROCRUSTES-DATA 'NIL
  :RAISE-POINTS 'NIL
  :LOWER-POINTS 'NIL
  :PRE-MERGE-TABLES (LIST)
  :ACMACS-A1-ANTIGENS 'NIL
  :ACMACS-A1-SERA 'NIL
  :ACMACS-B1-ANTIGENS '(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL)
  :ACMACS-B1-SERA '(NIL NIL NIL NIL NIL NIL)
  :DATE 'NIL
  :RBC-SPECIES 'NIL
  :LAB 'NIL
  :MINIMUM-COLUMN-BASIS 'NIL
  :ALLOW-TITERS-LESS-THAN 'NIL
  :TITER-TYPE 'NIL
  :PLOT-SPEC '(([antigen-name-AG or serum-name-SR] :CO "#ffdba5" :DS 4 :TR 0.0 :NM "BI/16190/68" :WN "BI/16190/68" :NS 10 :NC "#ffdba5" :SH "CIRCLE") ... *num-points)
  )
