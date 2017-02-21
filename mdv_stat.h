/* "mdv_stat.h" mdview���ơ����������Τ���Υإå��ե����� */

#ifndef _MDV_STAT_H_
#define _MDV_STAT_H_

#include "mdv_type.h"

typedef struct {
  /* ������Ϣ */
  int arg_version;           /* �����ν񼰤ΥС������(�ºݤϥ�ӥ����) */

  /* mdv_data��Ϣ */
  double length_unit;        /* Ĺ����ñ�� */
  double max_radius;         /* ɽ����ǽ����Ⱦ�� */
  int center_auto;           /* �ſ��μ�ư����ե饰 */
  MDV_3D center_coord;       /* �ſ��ν���� */
  MDV_3D period_length;      /* ��������Ĺ */
  MDV_3D period_x,           /* ���������٥��ȥ� */
         period_y,
         period_z;
  int fold_mode;             /* ���������ؤ��ޤ���ߤΥȥ��� */
  StringID background_color; /* �طʤο� */
  StringID mark_color;       /* mark����Ȥ��ο� */

  /* mdv_data��Ϣ�ɲ� */
  double euler_theta;        /* �����顼�� */
  double euler_psi;
  double euler_phi;

  /* �����Ϣ */
  double distance;           /* ���������а��� */
  int outline_mode;          /* �س�ɽ�� */

  /* UI��Ϣ */
  int window_size;           /* ɽ�������� */
  int frames;                /* ����®�� */
  int mark_mode;             /* γ�ҤΥޡ������� */
  /* TODO: mark���� */
  /* TODO: layer���� */
} MDV_Status;

extern MDV_Status *mdv_status;         /* ���ߤΥ��ơ����� */
extern MDV_Status *mdv_status_default; /* �ǡ����ե���������� */
extern MDV_Status *mdv_status_tmp;     /* �ƥ�ݥ����� */

void MDV_Init(void);

MDV_Status *MDV_StatusAlloc(void);
void MDV_StatusFree(MDV_Status *msp);
void MDV_StatusClear(MDV_Status *msp);
void MDV_StatusCopy(MDV_Status *dst, MDV_Status *src);
void MDV_Status_SetPeriodLength1(MDV_Status *msp, double length);
void MDV_Status_SetPeriodLength3(MDV_Status *msp,
  double x, double y, double z);
void MDV_Status_SetPeriodLength9(MDV_Status *msp,
  double xx, double xy, double xz, double yx, double yy, double yz,
  double zx, double zy, double zz);

#endif /* _MDV_STAT_H_ */
