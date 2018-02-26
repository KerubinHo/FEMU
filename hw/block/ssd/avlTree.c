#include "avlTree.h"



/******************************************************************** 
* 
* avlTreeHigh(TREE_NODE *pNode)
* 
* ¼ÆËãµ±Ç°Ê÷µÄ¸ß¶È
*  
* Returns         : Ê÷µÄ¸ß¶È
* 
*********************************************************************/ 
int avlTreeHigh(TREE_NODE *pNode)
{
	int lh=0,rh=0;
	if(!pNode)
		return 0;

	lh = avlTreeHigh(pNode->left_child);
	rh = avlTreeHigh(pNode->right_child);

	return (1+((lh>rh)?lh:rh));
}


/******************************************************************** 
* 
* avlTreeCheck(tAVLTree *pTree , TREE_NODE *pNode)
* 
* ¼ìÑéµ±Ç°µÄÓÐÐòÆ½ºâ¶þ²æÊ÷ÊÇ·ñÆ½ºâ
* ÊÇ·ñÊÇÓÐÐòµÄ£¬²¢ÇÒ¸÷½ÚµãµÄÖ¸ÕëÃ»ÓÐ
* ´íÎó
* 
* Returns         : 
* 			  1 : ±íÊ¾ÊÇÒ»¿ÃÍê±¸µÄÓÐÐòÆ½ºâ¶þ²æÊ÷	
*			  0 : ±íÊ¾ÊÇÒ»¿Ã ²»½¡¿µµÄ¶þ²æÊ÷
*                             ²»½¡¿µ¿ÉÄÜÊÇ²»Æ½ºâ£¬¿ÉÄÜÊÇÆ½ºâÒò×Ó
*                             ÓÐ´íÎó£¬Ò²¿ÉÄÜÊÇÖ¸Õë²»Æ¥Åä
*********************************************************************/ 
int avlTreeCheck(tAVLTree *pTree , TREE_NODE *pNode)
{
	int lh=0,rh=0;
	TREE_NODE *tree_root = AVL_NULL;

	if(!pTree || !pNode)
		return 0;

	lh = avlTreeHigh(pNode->left_child);
	rh = avlTreeHigh(pNode->right_child);
	if(pNode->bf != lh-rh)   /*Æ½ºâÒò×ÓÊÇÕýÈ·µÄ*/
		return 0;

	/*´æÔÚ×ó×ÓÊ÷£¬µ«ÊÇ×ó×ÓÊ÷Òª´óÓÚ×Ô¼º*/
	if(pNode->left_child && ((*pTree->keyCompare)(pNode , pNode->left_child))>=0)
		return 0;

	/*´æÔÚÓÒ×ÓÊ÷£¬µ«ÊÇÓÒ×ÓÊ÷Òª´óÓÚ×Ô¼º*/
	if(pNode->right_child && ((*pTree->keyCompare)(pNode , pNode->right_child))<=0)
		return 0;

	/*Èç¹û±¾½ÚµãµÄ¸¸Ç×½ÚµãÎª¿Õ£¬µ«ÊÇÊ÷¸ù²»ÊÇ×Ô¼º*/
	tree_root = pNode->tree_root;
	if(!tree_root && (pTree->pTreeHeader != pNode))
		return 0;

	if(tree_root)
	{
		/******************************
		*¸¸Ç×½ÚµãµÄ×óÓÒ×ÓÊ÷¶¼²»ÊÇ×Ô¼º»ò
		*¸¸Ç×½ÚµãµÄ×óÓÒ×ÓÊ÷¶¼ÊÇ×Ô¼º
		*******************************/
		if((tree_root->left_child != pNode && tree_root->right_child != pNode) ||
			(tree_root->left_child == pNode && tree_root->right_child == pNode))
			return 0;
	}

	/****************************
	*×ó×ÓÊ÷µÄ¸¸Ç×½Úµã²»ÊÇ×Ô¼º»òÕß
	*ÓÒ×ÓÊ÷µÄ¸¸Ç×½Úµã²»ÊÇ×Ô¼º
	*****************************/
	if((pNode->left_child && pNode->left_child->tree_root != pNode) ||
		(pNode->right_child && pNode->right_child->tree_root != pNode))
		return 0;

	if(pNode->left_child && !avlTreeCheck(pTree, pNode->left_child))
		return 0;

	if(pNode->right_child && !avlTreeCheck(pTree, pNode->right_child))
		return 0;

	return 1;
}


/******************************************************************** 
* 
* R_Rotate(TREE_NODE **ppNode)
* 
* ¶þ²æÊ÷ÒÔ*ppNodeÎª¸ù½Úµã£¬½øÐÐÓÒÐý×ª²Ù×÷
* 
* Returns         :  ÎÞ
*
*           ×ÖÄ¸ºóÃæµÄÊý×Ö±íÊ¾ÊÇÆ½ºâÒò×Ó
*
*             E2                C0  
*            / \               / \                    
*           C1  F0            B1  E0                  
*          / \       ==>     /   / \                        
*         B1  D0            A0  D0  F0                      
*        /                                                
*       A0                                                 
*                                              

*                                              
**********************************************************************/
static void R_Rotate(TREE_NODE **ppNode)
{
	TREE_NODE *l_child = AVL_NULL;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	l_child = pNode->left_child;
	pNode->left_child = l_child->right_child;
	if(l_child->right_child)
		l_child->right_child->tree_root = pNode;
	l_child->right_child = pNode;
	l_child->tree_root = pNode->tree_root;
	pNode->tree_root = l_child;
	(*ppNode) = l_child;
}


/******************************************************************** 
* 
* L_Rotate(TREE_NODE **ppNode)
* 
* ¶þ²æÊ÷ÒÔ*ppNodeÎª¸ù½Úµã£¬½øÐÐ×óÐý×ª²Ù×÷
* 
* Returns         :  ÎÞ
*                   
*          ×ÖÄ¸ºóÃæµÄÊý×Ö±íÊ¾ÊÇÆ½ºâÒò×Ó
*                             
*           B-2                  D0                
*          / \       ==>        / \                      
*         A0  D-1              B0  E0                     
*            / \              / \   \                   
*           C0  E-1          A0  C0  F0                  
*                \
*                 F0       
*******************************************************************/ 
static void L_Rotate(TREE_NODE **ppNode)
{
	TREE_NODE *r_child = AVL_NULL;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	r_child = pNode->right_child;
	pNode->right_child = r_child->left_child;
	if(r_child->left_child)
		r_child->left_child->tree_root = pNode;
	r_child->left_child = pNode;
	r_child->tree_root = pNode->tree_root;
	pNode->tree_root = r_child;
	(*ppNode) = r_child;
}


/******************************************************************** 
* 
* LeftBalance(TREE_NODE **ppNode)
* 
* ¶þ²æÊ÷*ppNode×ó±ßÆ«¸ß£¬Ê§È¥Æ½ºâ£¬½øÐÐ×óÆ½ºâ²Ù×÷
* 
* Returns         :  ÎÞ
********************************************************************/ 
static void LeftBalance(TREE_NODE **ppNode)
{
	TREE_NODE *left_child = AVL_NULL;
	TREE_NODE *right_child = AVL_NULL;
	TREE_NODE *tree_root = AVL_NULL;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	tree_root = pNode->tree_root;               /*±£´æµ±Ç°½ÚµãµÄ¸¸½Úµã*/
	left_child = pNode->left_child;             /*±£´æµ±Ç°½ÚµãµÄ×ó×ÓÊ÷*/
	switch(left_child->bf)
	{
	case LH_FACTOR:                             /*Èç¹û×ó×ÓÊ÷µÄÆ½ºâÒò×ÓÎª1£¬Ö¤Ã÷Ô­Ê¼×´Ì¬Îª×ó×ÓÊ÷±ÈÓÒ×ÓÊ÷¸ß*/
		pNode->bf = left_child->bf = EH_FACTOR; /*µ±Ç°½ÚµãµÄÆ½ºâÒò×ÓºÍ×ó×ÓÊ÷µÄÆ½ºâÒò×ÓÉèÎª0*/
		R_Rotate(ppNode);  /*µ±Ç°×ÓÊ÷ÓÒÐý*/
		break;
	case RH_FACTOR:                             /*Èç¹û×ó×ÓÊ÷µÄÆ½ºâÒò×ÓÎª-1£¬Ö¤Ã÷Ô­Ê¼×´Ì¬ÎªÓÒ×ÓÊ÷±È×ó×ÓÊ÷¸ß*/
		                                        /*ÄÇÃ´Æ½ºâÒò×ÓµÄ¼ÆËã¾ÍÐèÒª¸ù¾ÝÓÒ×ÓÊ÷µÄÆ½ºâÒò×ÓÀ´¼ÆËã*/
		right_child = left_child->right_child;
		switch(right_child->bf)
		{
		case LH_FACTOR:
			pNode->bf = RH_FACTOR;
			left_child->bf = EH_FACTOR;
			break;
		case EH_FACTOR:
			pNode->bf = left_child->bf = EH_FACTOR;
			break;
		case RH_FACTOR:
			pNode->bf = EH_FACTOR;
			left_child->bf = LH_FACTOR;
			break;
		}
		right_child->bf = EH_FACTOR;
		L_Rotate(&pNode->left_child);          /*½«±¾½ÚµãµÄ×ó×ÓÊ÷½øÐÐ×óÐý*/
		R_Rotate(ppNode);                      /*½«±¾½Úµã½øÐÐÓÒÐý*/
		break;
	case EH_FACTOR:                            /*×ó×ÓÊ÷µÄÆ½ºâÒò×ÓÎª0£¬±íÃ÷Ô­Ê¼×´Ì¬ÏÂ¸Ã×ÓÊ÷ÊÇÆ½ºâµÄ*/
		pNode->bf = LH_FACTOR;
		left_child->bf = RH_FACTOR;
		R_Rotate(ppNode);                     /*½«±¾½Úµã½øÐÐÓÒÐý*/
		break;
	}
	(*ppNode)->tree_root = tree_root;
	if(tree_root && tree_root->left_child == pNode)
		tree_root->left_child = *ppNode;
	if(tree_root && tree_root->right_child == pNode)
		tree_root->right_child = *ppNode;
}


/******************************************************************** 
* 
* RightBalance(TREE_NODE **ppNode)
* 
* ¶þ²æÊ÷*ppNodeÓÒ±ßÆ«¸ß£¬Ê§È¥Æ½ºâ£¬½øÐÐÓÒÆ½ºâ²Ù×÷
* 
* Returns         :  ÎÞ
********************************************************************/ 
static void RightBalance(TREE_NODE **ppNode)
{
	TREE_NODE *left_child = AVL_NULL;
	TREE_NODE *right_child = AVL_NULL;
	TREE_NODE *tree_root = AVL_NULL;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	tree_root = pNode->tree_root;
	right_child = pNode->right_child;
	switch(right_child->bf)
	{
	case RH_FACTOR:
		pNode->bf = right_child->bf = EH_FACTOR;
		L_Rotate(ppNode);
		break;
	case LH_FACTOR:
		left_child = right_child->left_child;
		switch(left_child->bf)
		{
		case RH_FACTOR:
			pNode->bf = LH_FACTOR;
			right_child->bf = EH_FACTOR;
			break;
		case EH_FACTOR:
			pNode->bf = right_child->bf = EH_FACTOR;
			break;
		case LH_FACTOR:
			pNode->bf = EH_FACTOR;
			right_child->bf = RH_FACTOR;
			break;
		}
		left_child->bf = EH_FACTOR;
		R_Rotate(&pNode->right_child);
		L_Rotate(ppNode);
		break;
	case EH_FACTOR:
		pNode->bf = RH_FACTOR;
		right_child->bf = LH_FACTOR;
		L_Rotate(ppNode);
		break;
	}
	(*ppNode)->tree_root = tree_root;
	if(tree_root && tree_root->left_child == pNode)
		tree_root->left_child = *ppNode;
	if(tree_root && tree_root->right_child == pNode)
		tree_root->right_child = *ppNode;
}


/******************************************************************** 
* 
* avlDelBalance(tAVLTree *pTree , TREE_NODE *pNode,int L_R_MINUS)
* 
* É¾³ý½ÚµãÖ®ºó£¬¶þ²æÊ÷¿ÉÄÜÒÑ¾­²»Æ½ºâÁË£¬´ËÊ±ÐèÒªÓÃ
* ´Ëº¯ÊýÀ´ÊµÏÖÉ¾³ý½ÚµãÖ®ºóµÄÆ½ºâ²Ù×÷¡£
* ×ÓÊ÷×ÔÆ½ºâµÄ¹ý³ÌÖÐ£¬¿ÉÄÜ³öÏÖÒ»ÖÖÇé¿ö£ºÄÇ¾ÍÊÇ×ÓÊ÷×ÔÉíÆ½ºâÁË£¬µ«ÊÇ
* ÆÆ»µÁË¸¸Ç×µÄÆ½ºâÐÔ£¬ËùÒÔ´Ëº¯Êý×öÁËµÝ¹éÆ½ºâ²Ù×÷£¬ÄÜ¹»Ê¹×îÐ¡²»Æ½ºâ
* ×ÓÊ÷Ö®ÉÏµÄËùÓÐ×æÏÈ½Úµã¶¼ÄÜ¹»Æ½ºâ¡£
* ×î»µ¿ÉÄÜµÄÇé¿ö¾ÍÊÇ´Ó×îÐ¡²»Æ½ºâ×ÖÊýµÄÊ÷¸ùÒ»Ö±µ½Õû¸ö´óÊ÷µÄÊ÷¸ù½Úµã
* Ö®¼äµÄËùÓÐ×ÓÊ÷¶¼²»Æ½ºâ£¬²»¹ýÕâÖÖ¸ÅÂÊºÜµÍ£¬Ò»°ãÀ´ËµµÝ¹é×î¶àÈý´Î¾Í
* ¿ÉÒÔÊµÏÖÕû¸öÊ÷µÄÆ½ºâ
* pTree 		:  ¶þ²æÊ÷Ö¸Õë
* pNode		:  ×îÐ¡²»Æ½ºâ×ÓÊ÷µÄ¸ù½Úµã
* L_R_MINUS	:  
*			LEFT_MINUS    -- ×ó±ßÊ§È¥Æ½ºâ£¬Ê÷¸ß¼õÉÙÁË1²ã
*                      RIGHT_MINUS  -- ÓÒ±ßÊ§È¥Æ½ºâ£¬Ê÷¸ß¼õÉÙÁË1²ã
*
* Returns         :  ÎÞ
******************************************************************/ 
static int avlDelBalance
(
 tAVLTree *pTree , 
 TREE_NODE *pNode,
 int L_R_MINUS
 )
{
	TREE_NODE *tree_root = AVL_NULL;

	tree_root = pNode->tree_root;
	if(L_R_MINUS == LEFT_MINUS)
	{
		switch(pNode->bf)
		{
		case EH_FACTOR:
			pNode->bf = RH_FACTOR;
			break;
		case RH_FACTOR:
			RightBalance(&pNode);
			if(!tree_root)
				pTree->pTreeHeader = pNode;
			if(pNode->tree_root && pNode->bf == EH_FACTOR)
			{
				if(pNode->tree_root->left_child == pNode)
					avlDelBalance(pTree , pNode->tree_root , LEFT_MINUS);
				else
					avlDelBalance(pTree , pNode->tree_root , RIGHT_MINUS);
			}
			break;
		case LH_FACTOR:
			pNode->bf = EH_FACTOR;
			if(pNode->tree_root && pNode->bf == EH_FACTOR)
			{
				if(pNode->tree_root->left_child == pNode)
					avlDelBalance(pTree , pNode->tree_root , LEFT_MINUS);
				else
					avlDelBalance(pTree , pNode->tree_root , RIGHT_MINUS);
			}
			break;
		}
	}

	if(L_R_MINUS == RIGHT_MINUS)
	{
		switch(pNode->bf)
		{
		case EH_FACTOR:
			pNode->bf = LH_FACTOR;
			break;
		case LH_FACTOR:
			LeftBalance(&pNode);
			if(!tree_root)
				pTree->pTreeHeader = pNode;
			if(pNode->tree_root && pNode->bf == EH_FACTOR)
			{
				if(pNode->tree_root->left_child == pNode)
					avlDelBalance(pTree , pNode->tree_root , LEFT_MINUS);
				else
					avlDelBalance(pTree , pNode->tree_root , RIGHT_MINUS);
			}
			break;
		case RH_FACTOR:
			pNode->bf = EH_FACTOR;
			if(pNode->tree_root && pNode->bf == EH_FACTOR)
			{
				if(pNode->tree_root->left_child == pNode)
					avlDelBalance(pTree , pNode->tree_root , LEFT_MINUS);
				else
					avlDelBalance(pTree , pNode->tree_root , RIGHT_MINUS);
			}
			break;
		}
	}

	return 1;
}


/******************************************************************** 
* 
* AVL_TREE_LOCK(tAVLTree *pTree , int timeout)
* 
* Ëø¶¨¶þ²æÊ÷£¬·ÀÖ¹¶à¸öÈÎÎñÍ¬Ê±¶ÔÊ÷½øÐÐÌí¼Ó»òÉ¾³ý²Ù×÷
* ´Ëº¯ÊýÊÇÕë¶ÔvxworksÏµÍ³µÄÀ©Õ¹£¬Èç¹û²»ÊÇvxworksÏµÍ³£¬ÄÇÃ´Ê÷µÄ»¥³â²Ù×÷
* ÐèÒª×Ô¶¨Òå
* timeout		: µÈ´ýÊ±¼ä£¬vxworks²Ù×÷ÏµÍ³ÀïÃætimeout=1¾ÍÊÇ1/60Ãë
*
* Returns         :  ÎÞ
*********************************************************************/ 
void AVL_TREE_LOCK
(
 tAVLTree *pTree,
 int timeout
 )
{
	if(!pTree
#if OS==3 || OS==4		
		|| !pTree->sem
#endif		
		)
		return;

#if OS==3 || OS==4
	semTake(pTree->sem,timeout);
#endif
	return;
}

/********************************************************************* 
* 
* AVL_TREE_UNLOCK(tAVLTree *pTree , int timeout)
* 
* ½â³ýËø¶¨
* ´Ëº¯ÊýÊÇÕë¶ÔvxworksÏµÍ³µÄÀ©Õ¹£¬Èç¹û²»ÊÇvxworksÏµÍ³£¬ÄÇÃ´Ê÷µÄ»¥³â²Ù×÷
* ÐèÒª×Ô¶¨Òå
* Returns         :  ÎÞ
*********************************************************************/ 
void AVL_TREE_UNLOCK
(
 tAVLTree *pTree
 )
{
	if(!pTree
#if OS==3 || OS==4		
		|| !pTree->sem
#endif		
		)
		return;

#if OS==3 || OS==4
	semGive(pTree->sem);
#endif
	return;
}

/******************************************************************** 
* 
* AVL_TREENODE_FREE(tAVLTree *pTree , TREE_NODE *pNode)
* 
* ÊÍ·ÅÒ»¸ö½ÚµãËùÕ¼ÓÃµÄÄÚ´æ£¬ÊÍ·Åº¯ÊýÐèÒªÓÃ»§×Ô¶¨Òå
* £¬²¢ÇÒÐèÒªÔÚ´´½¨¶þ²æÊ÷µÄÊ±ºò´«µÝ¸ø¶þ²æÊ÷
* 
* Returns         :  ÎÞ
*********************************************************************/ 
void AVL_TREENODE_FREE
(
 tAVLTree *pTree,
 TREE_NODE *pNode
 )
{
	if(!pTree || !pNode)
		return;

	(*pTree->free)(pNode);
	return ;
}

#ifdef ORDER_LIST_WANTED
/******************************************************************************** 
* 
* orderListInsert
*	(
*	tAVLTree *pTree,	      //Ê÷½á¹¹µÄÖ¸Õë	
*	TREE_NODE *pNode ,    //pInsertNode¼´½«²åÔÚ´Ë½ÚµãÇ°Ãæ»òºóÃæ
*	TREE_NODE *pInsertNode, //¼´½«²åÈëµÄ½ÚµãÖ¸Õë
*	int prev_or_next      // INSERT_PREV : ´ý²åÈë½Úµã²åÔÚpNodeÖ®Ç°
*                                           INSERT_NEXT : ´ý²åÈë½Úµã²åÔÚpNodeÖ®ºó           
*	)
* 
*   µ±Æ½ºâ¶þ²æÊ÷ÀïÔö¼ÓÒ»¸ö½ÚµãÖ®ºó£¬ÓÃ´Ëº¯ÊýÀ´¸üÐÂ
*  ÓÐÐòË«ÏòÁ´±í
* 
* Returns         :  1:³É¹¦  0:Ê§°Ü
*********************************************************************************/ 
static int orderListInsert
(
 tAVLTree *pTree,
 TREE_NODE *pNode , 
 TREE_NODE *pInsertNode,
 int prev_or_next
 )
{
	TREE_NODE *p = AVL_NULL;

	if(!pNode)
		return 0;

	if(prev_or_next == INSERT_PREV)
	{
		p = pNode->prev;
		if(p)	p->next = pInsertNode;
		else	pTree->pListHeader = pInsertNode;

		pInsertNode->prev = p;
		pInsertNode->next = pNode;
		pNode->prev = pInsertNode;
	}

	if(prev_or_next == INSERT_NEXT)
	{
		p = pNode->next;
		if(p)	p->prev = pInsertNode;
		else	pTree->pListTail = pInsertNode;

		pInsertNode->prev = pNode;
		pInsertNode->next = p;
		pNode->next = pInsertNode;
	}
	return 1;
}

/******************************************************************** 
* int orderListRemove
*	(
*	tAVLTree *pTree,    //Ê÷½á¹¹µÄÖ¸Õë
*	TREE_NODE *pRemoveNode   //¼´½«´ÓÓÐÐòË«ÏòÁ´±íÖÐÉ¾³ýµÄ½Úµã
*	)
* 
*   µ±Æ½ºâ¶þ²æÊ÷ÀïÉ¾³ýÒ»¸ö½ÚµãÖ®ºó£¬ÓÃ´Ëº¯ÊýÀ´¸üÐÂ
*  ÓÐÐòË«ÏòÁ´±í
* 
* Returns         :  1:³É¹¦   0:Ê§°Ü
********************************************************************/ 
static int orderListRemove
(
 tAVLTree *pTree,
 TREE_NODE *pRemoveNode
 )
{
	TREE_NODE *pPrev = AVL_NULL;
	TREE_NODE *pNext = AVL_NULL;

	if(!pRemoveNode)
		return 0;

	pPrev = pRemoveNode->prev;
	pNext = pRemoveNode->next;
	if(!pPrev && !pNext)
	{
		pTree->pListHeader = pTree->pListTail = AVL_NULL;
		return 1;
	}
	if(pPrev && pNext)
	{
		pPrev->next = pNext;
		pNext->prev = pPrev;
		return 1;
	}

	if(pPrev)
	{
		pPrev->next = AVL_NULL;
		pTree->pListTail = pPrev;
		return 1;
	}

	if(pNext)
	{
		pNext->prev = AVL_NULL;
		pTree->pListHeader = pNext;
		return 1;
	}
	else 
	{
		return 0;
	}
}


/******************************************************************** 
*      avlTreeFirst(tAVLTree *pTree)
* 
*   »ñÈ¡ÓÐÐòË«ÏòÁ´±íÀïÃæµÄµÚÒ»¸ö³ÉÔ±½Úµã
* 
* Returns         :  ³É¹¦:  µÚÒ»¸ö³ÉÔ±½ÚµãµÄÖ¸Õë
*                         Ê§°Ü:  AVL_NULL
*********************************************************************/ 
TREE_NODE *avlTreeFirst
(
 tAVLTree *pTree
 )
{
	if(!pTree)
		return AVL_NULL;

	if(!pTree->count || !pTree->pTreeHeader)
		return AVL_NULL;

	return (TREE_NODE *)pTree->pListHeader;
}


/******************************************************************** 
*      avlTreeLast(tAVLTree *pTree)
* 
*   »ñÈ¡ÓÐÐòË«ÏòÁ´±íÀïÃæµÄ×îºóÒ»¸ö³ÉÔ±½Úµã
* 
* Returns         :  ³É¹¦:  ×îºóÒ»¸ö³ÉÔ±½ÚµãµÄÖ¸Õë
*                         Ê§°Ü:  AVL_NULL
*********************************************************************/ 
TREE_NODE *avlTreeLast
(
 tAVLTree *pTree
 )
{
	if(!pTree)
		return AVL_NULL;

	if(!pTree->count || !pTree->pTreeHeader)
		return AVL_NULL;

	return (TREE_NODE *)pTree->pListTail;
}

/******************************************************************** 
*      avlTreeNext(TREE_NODE *pNode)
* 
*   »ñÈ¡ÓÐÐòË«ÏòÁ´±íÀïÃæµ±Ç°³ÉÔ±½ÚµãµÄºóÒ»¸ö½Úµã
* 
* Returns         :  ³É¹¦: ºóÒ»¸ö³ÉÔ±½ÚµãµÄÖ¸Õë
*                         Ê§°Ü:  AVL_NULL
*********************************************************************/ 
TREE_NODE *avlTreeNext
(
 TREE_NODE *pNode
 )
{
	if(!pNode)
		return AVL_NULL;

	return (TREE_NODE *)pNode->next;
}

/******************************************************************** 
*      avlTreePrev(TREE_NODE *pNode)
* 
*   »ñÈ¡ÓÐÐòË«ÏòÁ´±íÀïÃæµ±Ç°³ÉÔ±½ÚµãµÄÇ°Ò»¸ö½Úµã
* 
* Returns         :  ³É¹¦: Ç°Ò»¸ö³ÉÔ±½ÚµãµÄÖ¸Õë
*                         Ê§°Ü:  AVL_NULL
*********************************************************************/ 
TREE_NODE *avlTreePrev
(
 TREE_NODE *pNode
 )
{
	if(!pNode)
		return AVL_NULL;

	return (TREE_NODE *)pNode->prev;
}
#endif

/*****************************************************************************************
*      int avlTreeInsert
*	(
*	tAVLTree *pTree ,      //Ê÷½á¹¹µÄÖ¸Õë
*	TREE_NODE **ppNode ,  //´ý²åÈë½ÚµãËùÔÚµÄ×ÓÊ÷µÄÖ¸ÕëµÄÖ¸Õë
*	TREE_NODE *pInsertNode,  //´ý²åÈëµÄ½Úµã
*	int *growthFlag  //×ÓÊ÷ÊÇ·ñ³¤¸ßµÄ±êÖ¾ *growthFlag=1±íÊ¾³¤¸ß1²ã *growthFlag=0±íÊ¾Ã»ÓÐ
*	)
* 
*   ½«Ò»¸ö½Úµã²åÈëÒ»¿Å×ÓÊ÷Ö®ÖÐ£¬²åÈë¹ý³ÌÖ®ÖÐ¿ÉÄÜµ¼ÖÂ×ÓÊ÷²»
*  Æ½ºâ£¬´Ëº¯Êý»¹½«Ö´ÐÐµÝ¹éÆ½ºâ²Ù×÷£¬Ö±µ½ËùÓÐ×ÓÊ÷¾ùÆ½ºâÎªÖ¹
* 
* Returns         :  1:³É¹¦
*                         0:Ê§°Ü
******************************************************************************************/ 
static int avlTreeInsert
(
 tAVLTree *pTree , 
 TREE_NODE **ppNode , 
 TREE_NODE *pInsertNode,
 int *growthFlag
 )
{
	int compFlag = 0;
	TREE_NODE *pNode = (TREE_NODE *)(*ppNode);

	if(pTree->count == 0)
	{
		pTree->pTreeHeader = pInsertNode;
		pInsertNode->bf = EH_FACTOR;
		pInsertNode->left_child = pInsertNode->right_child = AVL_NULL;
		pInsertNode->tree_root = AVL_NULL;
#ifdef ORDER_LIST_WANTED
		pTree->pListHeader = pTree->pListTail = pInsertNode;
		pInsertNode->prev = pInsertNode->next = AVL_NULL;
#endif
		return 1;
	}

	compFlag = ((*pTree->keyCompare)(pNode , pInsertNode));
	if(!compFlag)
	{
		*growthFlag = 0;
		return 0;
	}

	if(compFlag < 0)
	{
		if(!pNode->left_child)
		{
			pNode->left_child = pInsertNode;
			pInsertNode->bf = EH_FACTOR;
			pInsertNode->left_child = pInsertNode->right_child = AVL_NULL;
			pInsertNode->tree_root = (TREE_NODE *)pNode;
#ifdef ORDER_LIST_WANTED
			orderListInsert(pTree,pNode, pInsertNode, INSERT_PREV);
#endif
			switch(pNode->bf)
			{
			case EH_FACTOR:
				pNode->bf = LH_FACTOR;
				*growthFlag = 1;
				break;
			case RH_FACTOR:
				pNode->bf = EH_FACTOR;
				*growthFlag = 0;
				break;
			}
		}
		else
		{
			if(!avlTreeInsert(pTree, &pNode->left_child,pInsertNode, growthFlag))
				return 0;

			if(*growthFlag)
			{
				switch(pNode->bf)
				{
				case LH_FACTOR:
					LeftBalance(ppNode);
					*growthFlag = 0;
					break;
				case EH_FACTOR:
					pNode->bf = LH_FACTOR;
					*growthFlag = 1;
					break;
				case RH_FACTOR:
					pNode->bf = EH_FACTOR;
					*growthFlag = 0;
					break;
				}
			}
		}
	}

	if(compFlag > 0)
	{
		if(!pNode->right_child)
		{
			pNode->right_child = pInsertNode;
			pInsertNode->bf = EH_FACTOR;
			pInsertNode->left_child = pInsertNode->right_child = AVL_NULL;
			pInsertNode->tree_root = (TREE_NODE *)pNode;
#ifdef ORDER_LIST_WANTED
			orderListInsert(pTree,pNode, pInsertNode, INSERT_NEXT);
#endif
			switch(pNode->bf)
			{
			case EH_FACTOR:
				pNode->bf = RH_FACTOR;
				*growthFlag = 1;
				break;
			case LH_FACTOR:
				pNode->bf = EH_FACTOR;
				*growthFlag = 0;
				break;
			}
		}
		else
		{
			if(!avlTreeInsert(pTree, &pNode->right_child,pInsertNode, growthFlag))
				return 0;

			if(*growthFlag)
			{
				switch(pNode->bf)
				{
				case LH_FACTOR:
					pNode->bf = EH_FACTOR;
					*growthFlag = 0;
					break;
				case EH_FACTOR:
					pNode->bf = RH_FACTOR;
					*growthFlag = 1;
					break;
				case RH_FACTOR:
					RightBalance(ppNode);
					*growthFlag = 0;
					break;
				}
			}
		}
	}

	return 1;
}


/******************************************************************** 
*      int avlTreeRemove
*	(
*	tAVLTree *pTree ,      //Ê÷½á¹¹µÄÖ¸Õë
*	TREE_NODE *pRemoveNode  //´ýÉ¾³ý½ÚµãµÄÖ¸Õë
*	)
* 
*   ´ÓÊ÷ÀïÃæÉ¾³ýÒ»¸ö½Úµã£¬´Ëº¯ÊýÄÜ¹»×öµÝ¹é²Ù×÷£¬ÄÜ¹»
*  Ñ­»·×ÔÆ½ºâ£¬Ê¹ËùÓÐÊÜÉ¾³ý½ÚµãÓ°Ïì¶øµ¼ÖÂ²»Æ½ºâµÄ×ÓÊ÷
*  ¶¼ÄÜ×ÔÆ½ºâ
* 
* Returns         :  1:³É¹¦
*                    0:Ê§°Ü
*                                                    
*          C               C                                                           
*         / \             / \                     C                                     
*        B   E    ==>    B  .F.      ==>         / \                                     
*       /   / \         /   / \                 B  .F.                                   
*      A   D   G       A   D   G               /   / \                                   
*             / \             / \             A   D   G                                 
*            F   H          .E.  H                     \                                  
*                                                       H                  
*      É¾³ýE½Úµã  ==> ÕÒµ½±ÈE´óÒ»µãµÄF ==>  É¾³ýE½Úµã£¬×ÔÆ½ºâ                                                           
*                     FºÍE»¥»»Ö¸Õë                                                
********************************************************************/ 
static int avlTreeRemove
(
 tAVLTree *pTree , 
 TREE_NODE *pRemoveNode
 )
{
	int compFlag = 0;
	TREE_NODE *tree_root = AVL_NULL;
	TREE_NODE *p = AVL_NULL;
	TREE_NODE *root_p = AVL_NULL;
	TREE_NODE swapNode;

	tree_root = pRemoveNode->tree_root;
	if(!pRemoveNode->left_child && !pRemoveNode->right_child)
	{
		if(!tree_root)
		{
			pTree->pTreeHeader = AVL_NULL;
#ifdef ORDER_LIST_WANTED
			pTree->pListHeader = pTree->pListTail = AVL_NULL;
#endif
			return 1;
		}
		else if(tree_root->left_child == pRemoveNode)
		{
#ifdef ORDER_LIST_WANTED
			orderListRemove(pTree, pRemoveNode);
#endif
			tree_root->left_child = AVL_NULL;
			avlDelBalance(pTree, tree_root , LEFT_MINUS);
		}
		else
		{
#ifdef ORDER_LIST_WANTED
			orderListRemove(pTree, pRemoveNode);
#endif
			tree_root->right_child = AVL_NULL;
			avlDelBalance(pTree, tree_root , RIGHT_MINUS);
		}
	}

	if(pRemoveNode->left_child && pRemoveNode->right_child)
	{
		TREE_NODE *prev = AVL_NULL;
		TREE_NODE *next = AVL_NULL;
		TREE_NODE *r_child = AVL_NULL;
		root_p = pRemoveNode;
		p = pRemoveNode->right_child;
		while(p->left_child)
		{
			root_p = p;
			p = p->left_child;
		}
		if(p == pRemoveNode->right_child)
		{
			p->tree_root = p;
			pRemoveNode->right_child = pRemoveNode;
		}
		swapNode = *p;
		prev = p->prev;
		next = p->next;
		*p = *pRemoveNode;
		p->prev = prev;
		p->next = next;
		prev = pRemoveNode->prev;
		next = pRemoveNode->next;
		*pRemoveNode = swapNode;
		pRemoveNode->prev = prev;
		pRemoveNode->next = next;
		if(!tree_root) 
			pTree->pTreeHeader = p;
		else if(tree_root->left_child == pRemoveNode)
			tree_root->left_child = p;
		else
			tree_root->right_child = p;

		if(p->left_child) 
			p->left_child->tree_root = p;
		if(p->right_child)  
			p->right_child->tree_root = p;

		if(pRemoveNode->left_child) 
			pRemoveNode->left_child->tree_root = pRemoveNode;
		if(pRemoveNode->right_child)  
			pRemoveNode->right_child->tree_root = pRemoveNode;

		if(root_p != pRemoveNode)
		{
			if(root_p->left_child == p)
				root_p->left_child = pRemoveNode;
			else 
				root_p->right_child = pRemoveNode;
		}

		return avlTreeRemove(pTree, pRemoveNode);
	}

	if(pRemoveNode->left_child)
	{
#ifdef ORDER_LIST_WANTED
		orderListRemove(pTree, pRemoveNode);
#endif
		if(!tree_root)
		{
			pTree->pTreeHeader = pRemoveNode->left_child;
			pRemoveNode->left_child->tree_root = AVL_NULL;
			return 1;
		}

		if(tree_root->left_child == pRemoveNode)
		{
			tree_root->left_child = pRemoveNode->left_child;
			pRemoveNode->left_child->tree_root= tree_root;
			avlDelBalance(pTree , tree_root , LEFT_MINUS);
		}
		else
		{
			tree_root->right_child = pRemoveNode->left_child;
			pRemoveNode->left_child->tree_root = tree_root;
			avlDelBalance(pTree , tree_root , RIGHT_MINUS);
		}

		return 1;
	}

	if(pRemoveNode->right_child)
	{
#ifdef ORDER_LIST_WANTED
		orderListRemove(pTree, pRemoveNode);
#endif
		if(!tree_root)
		{
			pTree->pTreeHeader = pRemoveNode->right_child;
			pRemoveNode->right_child->tree_root = AVL_NULL;
			return 1;
		}

		if(tree_root->left_child == pRemoveNode)
		{
			tree_root->left_child = pRemoveNode->right_child;
			pRemoveNode->right_child->tree_root = tree_root;
			avlDelBalance(pTree , tree_root , LEFT_MINUS);
		}
		else
		{
			tree_root->right_child = pRemoveNode->right_child;
			pRemoveNode->right_child->tree_root = tree_root;
			avlDelBalance(pTree , tree_root , RIGHT_MINUS);
		}

		return 1;
	}

	return 1;
}

/******************************************************************** 
*      int avlTreeLookup
*	(
*	tAVLTree *pTree,
*	TREE_NODE *pNode , 
*	TREE_NODE *pSearchKey
*	)
* 
*    µÝ¹é²éÕÒ¹Ø¼ü×Ö±È½ÏÍêÈ«Æ¥ÅäµÄ½Úµã£¬±È½Ïº¯ÊýÊÇÔÚ
*     Ê÷´´½¨µÄÊ±ºò¾ÍÖ¸¶¨ºÃµÄ
*
* Returns         :  1:³É¹¦
*                         0:Ê§°Ü
*********************************************************************/ 
static TREE_NODE *avlTreeLookup
(
 tAVLTree *pTree,
 TREE_NODE *pNode , 
 TREE_NODE *pSearchKey
 )
{
	int compFlag = 0;
	if(!pTree || !pNode)
		return AVL_NULL;

	compFlag = (*pTree->keyCompare)(pNode , pSearchKey);
	if(!compFlag)
		return (TREE_NODE *)pNode;

	if(compFlag>0) pNode = pNode->right_child;
	else pNode = pNode->left_child;

	return (TREE_NODE *)avlTreeLookup(pTree, pNode, pSearchKey);
}


/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
¡ïÃèÊö            : ´´½¨Ò»¿ÅÓÐÐòÆ½ºâ¶þ²æÊ÷
¡ï²ÎÊýÃèÊö: 
keyCompareFunc:±È½ÏÁ½¸ö½ÚµãµÄ´óÐ¡(¹Ø¼ü×ÖµÄ±È½Ï)
¡ï·µ»ØÖµ      :
³É¹¦ :   Æ½ºâ¶þ²æÊ÷µÄÖ¸Õë
Ê§°Ü :   ¿ÕÖ¸Õë
*******************************************************************/
tAVLTree *avlTreeCreate(int *keyCompareFunc,int *freeFunc)
{
	tAVLTree *pTree = (tAVLTree *)0;

	if(!keyCompareFunc || !freeFunc)
		return (tAVLTree *)0;

	pTree = (tAVLTree *)malloc(sizeof(tAVLTree));
	
	if(pTree != (tAVLTree *)0)
	{
		memset((void *)pTree , 0 , sizeof(tAVLTree));
		pTree->keyCompare = (void *)keyCompareFunc;
		pTree->free = (void *)freeFunc;
#ifdef ORDER_LIST_WANTED
		pTree->pListHeader = pTree->pListTail = AVL_NULL;
#endif

#if OS==3 || OS==4 
		pTree->sem = semBCreate(0 , 1);
		if(!pTree->sem)
		{
			free((void *)pTree);
			return (tAVLTree *)0;
		}
#endif
	}

	return (tAVLTree *)pTree;
}

/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
¡ïÃèÊö            :  É¾³ýÒ»¸ö½Úµã

¡ï²ÎÊýÃèÊö: 
pTree:Ê÷½á¹¹µÄÖ¸Õë
pDelNode : ´ýÉ¾³ýµÄ½ÚµãÖ¸Õë
¡ï·µ»ØÖµ      :
³É¹¦ :  1
Ê§°Ü :   0
*******************************************************************/
int avlTreeDel( tAVLTree *pTree ,TREE_NODE *pDelNode)
{
	int ret = 0;

	if(!pTree || !pDelNode || !pTree->count)
		return 0;

	ret = avlTreeRemove(pTree, pDelNode);
	if(ret)
		pTree->count--;

	return 1;
}


/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
¡ïÃèÊö            : ´Ý»ÙÒ»¿ÅÆ½ºâ¶þ²æÊ÷£¬²¢ÊÍ·ÅËùÓÐ³ÉÔ±½ÚµãÕ¼ÓÃµÄÄÚ´æ
ÊÍ·ÅÄÚ´æµÄº¯ÊýÔÚ´´½¨Ê÷µÄÊ±ºòÒÑ¾­Ö¸¶¨ºÃ
¡ï²ÎÊýÃèÊö: 
pTree:Ê÷½á¹¹µÄÖ¸Õë
¡ï·µ»ØÖµ      :
³É¹¦ :  1
Ê§°Ü :   0
********************************************************************/
int avlTreeDestroy
(
 tAVLTree *pTree
 )
{
	TREE_NODE *pNode = AVL_NULL;
	if(!pTree)
		return 0;

	while(pNode = pTree->pTreeHeader)
	{
		avlTreeDel(pTree,pNode);
		AVL_TREENODE_FREE(pTree, pNode);
	}

	if(!pTree->count || !pTree->pTreeHeader)
	{
#if OS==3 || OS==4
		semDelete(pTree->sem);
#endif
		free((void *)pTree);
		return 1;
	}

	return 0;
}


/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
¡ïÃèÊö            : Çå¿ÕÒ»¿ÅÊ÷£¬ÊÍ·ÅËùÓÐ³ÉÔ±½ÚµãÕ¼ÓÃµÄÄÚ´æ£¬
µ«ÊÇ²»ÊÍ·ÅÊ÷½á¹¹ËùÕ¼ÓÃµÄÄÚ´æ
¡ï²ÎÊýÃèÊö: 
pTree:Ê÷½á¹¹µÄÖ¸Õë
¡ï·µ»ØÖµ      :
³É¹¦ :  1
Ê§°Ü :   0
********************************************************************/
int avlTreeFlush
(
 tAVLTree *pTree
 )
{
	TREE_NODE *pNode = AVL_NULL;

	if(!pTree)
		return 0;

	if(!pTree->count || !pTree->pTreeHeader)
		return 1;

	while(pNode = pTree->pTreeHeader)
	{
		avlTreeDel(pTree,pNode);
		AVL_TREENODE_FREE(pTree, pNode);
	}

	return 0;
}


/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
¡ïÃèÊö            :  Ôö¼ÓÒ»¸ö½Úµã

¡ï²ÎÊýÃèÊö: 
pTree:Ê÷½á¹¹µÄÖ¸Õë
pInsertNode : ´ýÌí¼ÓµÄ½ÚµãÖ¸Õë
¡ï·µ»ØÖµ      :
³É¹¦ :  1
Ê§°Ü :   0
*******************************************************************/
int avlTreeAdd
(
 tAVLTree *pTree , 
 TREE_NODE *pInsertNode
 )
{
	int growthFlag=0 , ret = 0;

	if(!pTree || !pInsertNode)
		return 0;

	ret = avlTreeInsert(pTree , &pTree->pTreeHeader , pInsertNode , &growthFlag);
	if(ret)
		pTree->count++;
	return ret;
}



/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
¡ïÃèÊö            : ¸ù¾Ý¹Ø¼ü×Ö½á¹¹À´²éÑ¯Ò»¸ö½ÚµãÊÇ·ñ´æÔÚ

¡ï²ÎÊýÃèÊö: 
pTree:Ê÷½á¹¹µÄÖ¸Õë
pKeyNode : ¹Ø¼ü×Ö½á¹¹Ö¸Õë
¡ï·µ»ØÖµ      :
³É¹¦ :  ²éÕÒµ½µÄ½ÚµãÖ¸Õë
Ê§°Ü :   AVL_NULL
********************************************************************/
TREE_NODE *avlTreeFind
(
 tAVLTree *pTree,
 TREE_NODE *pKeyNode
 )
{
	if(!pTree || !pTree->count || !pTree->pTreeHeader)
		return AVL_NULL;

	return (TREE_NODE *)avlTreeLookup(pTree, pTree->pTreeHeader , pKeyNode);
}

/*******************************************************************/
/**************************AVL TREE API*****************************/
/*******************************************************************/
/*
¡ïÃèÊö            : »ñÈ¡Ê÷ÀïÃæµÄËùÓÐ½Úµã×ÜÊý

¡ï²ÎÊýÃèÊö: 
pTree:Ê÷½á¹¹µÄÖ¸Õë
¡ï·µ»ØÖµ      :
Ê÷ÀïÃæµÄ½Úµã³ÉÔ±×ÜÊý
********************************************************************/
unsigned int avlTreeCount
(
 tAVLTree *pTree
 )
{
	if(!pTree)
		return 0;

	return pTree->count;
}


